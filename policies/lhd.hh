#pragma once

#include <stdio.h>
#include <cmath>
#include <map>
#include <vector>
#include "tsl/robin_map.h"

class lhd
{
    // info we track about each object
    struct Tag {
        uint64_t timestamp;
        uint64_t lastHitAge;
        uint64_t lastLastHitAge;
        uint32_t app;
        
        candidate_t id;
        float size; // stored redundantly with cache
        bool explorer;
    };

    // info we track about each class of objects
    struct Class {
        vector<float> hits;
        vector<float> evictions;
        float totalHits = 0;
        float totalEvictions = 0;

        vector<float> hitDensities;
    };

    // CONSTANTS ///////////////////////////

    // how to sample candidates; can significantly impact hit
    // ratio. want a value at least 32; diminishing returns after
    // that.
    const uint32_t ASSOCIATIVITY = 32;

    // since our cache simulator doesn't bypass objects, we always
    // consider the last ADMISSIONS objects as eviction candidates
    // (this is important to avoid very large objects polluting the
    // cache.) alternatively, you could do bypassing and randomly
    // admit objects as "explorers" (see below).
    const uint32_t ADMISSIONS = 8;

    // escape local minima by having some small fraction of cache
    // space allocated to objects that aren't evicted. 1% seems to be
    // a good value that has limited impact on hit ratio while quickly
    // discovering the shape of the access pattern.
    static constexpr float EXPLORER_BUDGET_FRACTION = 0.01;
    static constexpr uint32_t EXPLORE_INVERSE_PROBABILITY = 32;

    // these parameters determine how aggressively to classify objects.
    // diminishing returns after a few classes; 16 is safe.
    static constexpr uint32_t HIT_AGE_CLASSES = 16;
    static constexpr uint32_t APP_CLASSES = 16;
    static constexpr uint32_t NUM_CLASSES = HIT_AGE_CLASSES * APP_CLASSES;
    
    // these parameters are tuned for simulation performance, and hit
    // ratio is insensitive to them at reasonable values (like these)
    static constexpr float AGE_COARSENING_ERROR_TOLERANCE = 0.01;
    static constexpr uint64_t MAX_AGE = 20000;
    static constexpr uint64_t ACCS_PER_RECONFIGURATION = (1 << 20);
    static constexpr float EWMA_DECAY = 0.9;

    // verbose debugging output?
    static constexpr bool DUMP_RANKS = false;

    // FIELDS //////////////////////////////
    cache::Cache *cache;

    // object metadata; indices maps object id -> metadata
    vector<Tag> tags;
    vector<Class> classes;
    std::unordered_map<candidate_t, uint64_t> indices;

    // time is measured in # of requests
    uint64_t timestamp = 0;
    
    uint64_t nextReconfiguration = 0;
    int numReconfigurations = 0;
    
    // how much to shift down age values; initial value doesn't really
    // matter, but must be positive. tuned in adaptAgeCoarsening() at
    // the beginning of the trace using ewmaNumObjects.
    uint64_t ageCoarseningShift = 10;
    float ewmaNumObjects = 0;
    float ewmaNumObjectsMass = 0.;

    // how many objects had age > max age (this should almost never
    // happen -- if you observe non-neglible overflows, something has
    // gone wrong with the age coarsening!!!)
    uint64_t overflows = 0;

    misc::Rand rand;

    // see ADMISSIONS above
    vector<candidate_t> recentlyAdmitted;
    int recentlyAdmittedHead = 0;
    float ewmaVictimHitDensity = 0;

    int64_t explorerBudget = 0;
    
    // METHODS /////////////////////////////

    // returns something like log(maxAge - age)
    inline uint32_t hitAgeClass(uint64_t age) const {
        if (age == 0) { return HIT_AGE_CLASSES - 1; }
        uint32_t log = 0;
        while (age < MAX_AGE && log < HIT_AGE_CLASSES - 1) {
            age <<= 1;
            log += 1;
        }
        return log;
    }

    inline uint32_t getClassId(const Tag& tag) const {
        uint32_t hitAgeId = hitAgeClass(tag.lastHitAge + tag.lastLastHitAge);
        return tag.app * HIT_AGE_CLASSES + hitAgeId;
    }
    
    inline Class& getClass(const Tag& tag) {
        return classes[getClassId(tag)];
    }

    inline uint64_t getAge(Tag tag) {
        uint64_t age = (timestamp - (uint64_t)tag.timestamp) >> ageCoarseningShift;

        if (age >= MAX_AGE) {
            ++overflows;
            return MAX_AGE - 1;
        } else {
            return (uint64_t) age;
        }
    }

    inline float getHitDensity(const Tag& tag) {
        auto age = getAge(tag);
        if (age == MAX_AGE-1) { return std::numeric_limits<float>::lowest(); }
        auto& cl = getClass(tag);
        float density = cl.hitDensities[age] / tag.size;
        if (tag.explorer) { density += 1.; }
        return density;
    }
        
    void reconfigure();
    void adaptAgeCoarsening();
    void updateClass(Class& cl);
    void modelHitDensity();
    void dumpClassRanks(Class& cl);
    
public:
    LHD(int _associativity, int _admissions, Cache* _cache):
    ASSOCIATIVITY(_associativity),
    ADMISSIONS(_admissions),
    recentlyAdmitted(_admissions, {-1, -1}) {
        nextReconfiguration = ACCS_PER_RECONFIGURATION;
        explorerBudget = _cache->availableCapacity * EXPLORER_BUDGET_FRACTION;
        
        for (uint32_t i = 0; i < NUM_CLASSES; i++) {
            classes.push_back(Class());
            auto& cl = classes.back();
            cl.hits.resize(MAX_AGE, 0);
            cl.evictions.resize(MAX_AGE, 0);
            cl.hitDensities.resize(MAX_AGE, 0);
        }

        // Initialize policy to ~GDSF by default.
        for (uint32_t c = 0; c < NUM_CLASSES; c++) {
            for (uint64_t a = 0; a < MAX_AGE; a++) {
                classes[c].hitDensities[a] =
                    1. * (c + 1) / (a + 1);
            }
        }
    }


    ~LHD() {}

    // called whenever and object is referenced
    void update(candidate_t id, const parser::Request& req);

    // called when an object is evicted
    void replaced(candidate_t id);

    // called to find a victim upon a cache miss
    candidate_t rank(const parser::Request& req);

    void dumpStats(cache::Cache* cache) { }
};



candidate_t LHD::rank(const parser::Request& req) {
    uint64_t victim = -1;
    float victimRank = std::numeric_limits<float>::max();

    // Sample few candidates early in the trace so that we converge
    // quickly to a reasonable policy.
    //
    // This is a hack to let us have shorter warmup so we can evaluate
    // a longer fraction of the trace; doesn't belong in a real
    // system.
    uint32_t candidates =
        (numReconfigurations > 50)?
        ASSOCIATIVITY : 8;

    for (uint32_t i = 0; i < candidates; i++) {
        auto idx = rand.next() % tags.size();
        auto& tag = tags[idx];
        float rank = getHitDensity(tag);

        if (rank < victimRank) {
            victim = idx;
            victimRank = rank;
        }
    }

    for (uint32_t i = 0; i < ADMISSIONS; i++) {
        auto itr = indices.find(recentlyAdmitted[i]);
        if (itr == indices.end()) { continue; }

        auto idx = itr->second;
        auto& tag = tags[idx];
        assert(tag.id == recentlyAdmitted[i]);
        float rank = getHitDensity(tag);

        if (rank < victimRank) {
            victim = idx;
            victimRank = rank;
        }
    }

    assert(victim != (uint64_t)-1);

    ewmaVictimHitDensity = EWMA_DECAY * ewmaVictimHitDensity + (1 - EWMA_DECAY) * victimRank;

    return tags[victim].id;
}

void LHD::update(candidate_t id, const parser::Request& req) {
    auto itr = indices.find(id);
    bool insert = (itr == indices.end());
        
    Tag* tag;
    if (insert) {
        tags.push_back(Tag{});
        tag = &tags.back();
        indices[id] = tags.size() - 1;
        
        tag->lastLastHitAge = MAX_AGE;
        tag->lastHitAge = 0;
        tag->id = id;
    } else {
        tag = &tags[itr->second];
        assert(tag->id == id);
        auto age = getAge(*tag);
        auto& cl = getClass(*tag);
        cl.hits[age] += 1;

        if (tag->explorer) { explorerBudget += tag->size; }
        
        tag->lastLastHitAge = tag->lastHitAge;
        tag->lastHitAge = age;
    }

    tag->timestamp = timestamp;
    tag->app = req.appId % APP_CLASSES;
    tag->size = req.size();

    // with some probability, some candidates will never be evicted
    // ... but limit how many resources we spend on doing this
    bool explore = (rand.next() % EXPLORE_INVERSE_PROBABILITY) == 0;
    if (explore && explorerBudget > 0 && numReconfigurations < 50) {
        tag->explorer = true;
        explorerBudget -= tag->size;
    } else {
        tag->explorer = false;
    }

    // If this candidate looks like something that should be
    // evicted, track it.
    if (insert && !explore && getHitDensity(*tag) < ewmaVictimHitDensity) {
        recentlyAdmitted[recentlyAdmittedHead++ % ADMISSIONS] = id;
    }
    
    ++timestamp;

    if (--nextReconfiguration == 0) {
        reconfigure();
        nextReconfiguration = ACCS_PER_RECONFIGURATION;
        ++numReconfigurations;
    }
}

void LHD::replaced(candidate_t id) {
    auto itr = indices.find(id);
    assert(itr != indices.end());
    auto index = itr->second;

    // Record stats before removing item
    auto& tag = tags[index];
    assert(tag.id == id);
    auto age = getAge(tag);
    auto& cl = getClass(tag);
    cl.evictions[age] += 1;

    if (tag.explorer) { explorerBudget += tag.size; }

    // Remove tag for replaced item and update index
    indices.erase(itr);
    tags[index] = tags.back();
    tags.pop_back();

    if (index < tags.size()) {
        indices[tags[index].id] = index;
    }
}

void LHD::reconfigure() {
    float totalHits = 0;
    float totalEvictions = 0;
    for (auto& cl : classes) {
        updateClass(cl);
        totalHits += cl.totalHits;
        totalEvictions += cl.totalEvictions;
    }

    adaptAgeCoarsening();
        
    modelHitDensity();

    // Just printfs ...
    for (uint32_t c = 0; c < classes.size(); c++) {
        auto& cl = classes[c];
        // printf("Class %d | hits %g, evictions %g, hitRate %g\n",
        //        c,
        //        cl.totalHits, cl.totalEvictions,
        //        cl.totalHits / (cl.totalHits + cl.totalEvictions));

        dumpClassRanks(cl);
    }
    printf("LHD | hits %g, evictions %g, hitRate %g | overflows %lu (%g) | cumulativeHitRate nan\n",
           totalHits, totalEvictions,
           totalHits / (totalHits + totalEvictions),
           overflows,
           1. * overflows / ACCS_PER_RECONFIGURATION);

    overflows = 0;
}

void LHD::updateClass(Class& cl) {
    cl.totalHits = 0;
    cl.totalEvictions = 0;

    for (uint64_t age = 0; age < MAX_AGE; age++) {
        cl.hits[age] *= EWMA_DECAY;
        cl.evictions[age] *= EWMA_DECAY;

        cl.totalHits += cl.hits[age];
        cl.totalEvictions += cl.evictions[age];
    }
}

void LHD::modelHitDensity() {
    for (uint32_t c = 0; c < classes.size(); c++) {
        float totalEvents = classes[c].hits[MAX_AGE-1] + classes[c].evictions[MAX_AGE-1];
        float totalHits = classes[c].hits[MAX_AGE-1];
        float lifetimeUnconditioned = totalEvents;

        // we use a small trick here to compute expectation in O(N) by
        // accumulating all values at later ages in
        // lifetimeUnconditioned.
        
        for (uint64_t a = MAX_AGE - 2; a < MAX_AGE; a--) {
            totalHits += classes[c].hits[a];
            
            totalEvents += classes[c].hits[a] + classes[c].evictions[a];

            lifetimeUnconditioned += totalEvents;

            if (totalEvents > 1e-5) {
                classes[c].hitDensities[a] = totalHits / lifetimeUnconditioned;
            } else {
                classes[c].hitDensities[a] = 0.;
            }
        }
    }
}

void LHD::dumpClassRanks(Class& cl) {
    if (!DUMP_RANKS) { return; }
    
    // float objectAvgSize = cl.sizeAccumulator / cl.totalHits; // + cl.totalEvictions);
    float objectAvgSize = 1. * used_ / map_.size();
    float left;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Ranks for avg object (" << objectAvgSize << "): ";
    for (uint64_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      float density = cl.hitDensities[a] / objectAvgSize;
      rankStr << density << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Hits: ";
    for (uint32_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      rankStr << cl.hits[a] << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Evictions: ";
    for (uint32_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      rankStr << cl.evictions[a] << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;
}

// this happens very rarely!
//
// it is simple enough to set the age coarsening if you know roughly
// how big your objects are. to make LHD run on different traces
// without needing to configure this, we set the age coarsening
// automatically near the beginning of the trace.
void LHD::adaptAgeCoarsening() {
    ewmaNumObjects *= EWMA_DECAY;
    ewmaNumObjectsMass *= EWMA_DECAY;

    ewmaNumObjects += map_.size();
    ewmaNumObjectsMass += 1.;

    float numObjects = ewmaNumObjects / ewmaNumObjectsMass;

    float optimalAgeCoarsening = 1. * numObjects / (AGE_COARSENING_ERROR_TOLERANCE * MAX_AGE);

    // Simplify. Just do this once shortly after the trace starts and
    // again after 25 iterations. It only matters that we are within
    // the right order of magnitude to avoid tons of overflows.
    if (numReconfigurations == 5 || numReconfigurations == 25) {
        uint32_t optimalAgeCoarseningLog2 = 1;

        while ((1 << optimalAgeCoarseningLog2) < optimalAgeCoarsening) {
            optimalAgeCoarseningLog2 += 1;
        }

        int32_t delta = optimalAgeCoarseningLog2 - ageCoarseningShift;
        ageCoarseningShift = optimalAgeCoarseningLog2;
        
        // increase weight to delay another shift for a while
        ewmaNumObjects *= 8;
        ewmaNumObjectsMass *= 8;
        
        // compress or stretch distributions to approximate new scaling
        // regime
        if (delta < 0) {
            // stretch
            for (auto& cl : classes) {
                for (uint64_t a = MAX_AGE >> (-delta); a < MAX_AGE - 1; a++) {
                    cl.hits[MAX_AGE - 1] += cl.hits[a];
                    cl.evictions[MAX_AGE - 1] += cl.evictions[a];
                }
                for (uint64_t a = MAX_AGE - 2; a < MAX_AGE; a--) {
                    cl.hits[a] = cl.hits[a >> (-delta)] / (1 << (-delta));
                    cl.evictions[a] = cl.evictions[a >> (-delta)] / (1 << (-delta));
                }
            }
        } else if (delta > 0) {
            // compress
            for (auto& cl : classes) {
                for (uint64_t a = 0; a < MAX_AGE >> delta; a++) {
                    cl.hits[a] = cl.hits[a << delta];
                    cl.evictions[a] = cl.evictions[a << delta];
                    for (int i = 1; i < (1 << delta); i++) {
                        cl.hits[a] += cl.hits[(a << delta) + i];
                        cl.evictions[a] += cl.evictions[(a << delta) + i];
                    }
                }
                for (uint64_t a = (MAX_AGE >> delta); a < MAX_AGE - 1; a++) {
                    cl.hits[a] = 0;
                    cl.evictions[a] = 0;
                }
            }
        }
    }
    
    printf("LHD at %lu | ageCoarseningShift now %lu | num objects %g | optimal age coarsening %g | current age coarsening %g\n",
           timestamp, ageCoarseningShift,
           numObjects,
           optimalAgeCoarsening,
           1. * (1 << ageCoarseningShift));
}







class lhd
{
    typedef tsl::robin_map<uint32_t, uint64_t> mit;
    typedef std::pair<const double, kfs> ckfs;

    size_t used_;
    size_t capacity_;
    size_t counter_;
    size_t hits_;

    double min_score_;
    
    vector<kfs> order_;
    tsl::robin_map<uint32_t, uint64_t> map_;

    void evict(ssize_t size)
    {
    }

public:
    gdsfr_cache(size_t capacity):
    used_(0),
    capacity_(capacity),
    counter_(0),
    hits_(0),
    min_score_(0)
    {}
    
    void insert(uint32_t key, uint32_t size)
    {
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
    }

    void clear()
    {
        map_.clear();
        order_.clear();
        min_score_ = 0;
    }

    void print()
    {
        printf("hit rate: %f\n", double(hits_)/double(counter_));
    }
};


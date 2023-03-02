Actions
- Keep
- Evict

State
- Age (binned)
- LRU4 (binned)
- Hits prev cumulative (binned)
- Hits prev section (binned)
- Size (binned)

percentile for each to the ^ of 5

void calc_pctile125x(float* pctile, float* x, uint32_t ct)
{
    int bkp = ct >> 3;

    sort(x, x + ct);

    pctile[0] = x[bkp];
    pctile[1] = x[bkp << 1];
    pctile[2] = x[bkp * 3];
    pctile[3] = x[bkp << 2];
    pctile[4] = x[bkp * 5];
    pctile[5] = x[bkp * 6];
    pctile[6] = x[bkp * 7];
}

void get_pctile125x(float* pctile, float* x, uint32_t ct)
{
    int bkp = ct >> 3;

    sort(x, x + ct);

    if (val < pctile[0]) return 0;
    if (val < pctile[1]) return 1;
    if (val < pctile[2]) return 2;
    if (val < pctile[3]) return 3;
    if (val < pctile[4]) return 4;
    if (val < pctile[5]) return 5;
    if (val < pctile[6]) return 6;

    return 7;
}

int calc_state(uint32_t a, uint32_t l, uint32_t c, uint32_t b, uint32_t s)
{
    return \
    a * (8 * 8 * 8 * 8) + \
    l * (8 * 8 * 8) + \
    c * (8 * 8) + \
    b * (8) + \
    s;
}


struct qcache_action
{
    int key;
};

struct qcache_entry
{
    int key;
    int lruk;
    int hits_prev;
    int size;
    int hits;
};

vector<qcache_entry> q;

// sort cache entries
sort(q.begin(),q.end());
// get cumulative size for all objects
// set keep to false (after a certain point), which means those after should have been evicted
for(int i=1;i<q.size();++i){q[i].size+=q[i-1].size;bkp=(q[i].size>limit && bkp==-1) i ? bkp;}

//

class qlearning_cache
{
    typedef tsl::robin_map<uint32_t, uint64_t> ckfs;
    typedef std::pair<const double, kfs> ckfs;
    typedef std::pair<float, int> skey;

    int size_pctile[8];
    int lru4_pctile[8];
    int hits_pctile[8];
    int hitc_pctile[8];


    size_t used_;
    size_t capacity_;
    size_t counter_;
    size_t hits_;

    double min_score_;
    
    // add to this when object is evicted
    // clear this when counter % 65536 == 0
    // each pair is <key, (didEvict << 31) | hits after eviction>
    vector<pair<int,int>> evicted_;
    vector<kfs> order_;
    tsl::robin_map<uint32_t, uint64_t> map_;

    void clear_references()
    {
        for (auto ev : evicted_) {
            auto& obj = order_[map_[ev.first]];
            update_q(obj);

            if (obj.prev_evict) {
                
            }
        }
    }

    void update_q(vector<skey>& candidates, int& state)
    {
        // Has object been seen before
        // NO: 0 points
        int reward = 0;
        // perhaps, 
        int next_state = get_state(obj);

        if (obj.seen) {
            if (obj.hitct[2]==0) {
                reward = (obj.prev_evict==0) ? ((obj.considered) ? -20 : 0) : 20;
            } else {
                reward = (obj.prev_evict==0) ? ((obj.considered) ? 200 : 40) : -200;
            }
        }

        qtable[state][action] = reward + gamma * max(qtable[next_state], action_ct);

        state = next_state;
    }

    void evict()
    {
        ssize_t sz = mint(size << 4, capacity_);
        vector<kom> candidates;
        set<int, char> exploit;
        candidates.reserve(16384);

        while (sz >= 0) {
            auto idx = random32(candidates.size());
            auto& obj = order_[idx];

            if (random32() > (epsilon * UINT32_MAX)) {
                /*randomly see if we are to evict or keep*/
                auto it = exploit.find(obj.key);
                if (it == exploit.end()) {
                    if (random32() & 1) {exploit.insert(obj.key, 1);} // evict
                    else {exploit.insert(obj.key, 0);} // keep
                }
            } else {
                // otherwise pick the statistically-best option, which i guess for this would
                // be to get the q score for each, then sort them?
                candidates.push_back({q[calc_state(obj)], obj});
            }
        }

        std::sort(candidates.begin(), candidates.end(),
        [](const kom& a, const kom& b) {
            return a.first < b.first;
        });

        for (auto pit : candidates) {
            auto& obj = pit.second->second;
            size -= obj.size;
            used_ -= obj.size;
            map_.erase(obj.key);
            order_.erase(pit.second);

            if (size <= 0)
                break;
        }
    }

    void train_cache()
    {
        // training loop
        for (int i = 0; i < epochs; ++i) {
            state, reward, done = env.reset();
            steps = 0;
            
            printf("[epoch %d/%d]", i+1, epochs);

            for (int j = 0; j < cache_actions; ++j) {
                kfs k = actions[i];

                auto it = 3;
                if () {
                    evict();
                } else {

                }

            }

            // The more we learn, the less we take random actions
            epsilon -= decay*epsilon

            print("\nDone in", steps, "steps".format(steps))
            time.sleep(0.8)
        }
    }

public:

    qlearning_cache(size_t capacity, int N):
    used_(0),
    capacity_(capacity),
    timestamp_(0),
    hits_(0)
    {
        vector<act> actions;

        // QTable : contains the Q-Values for every (state,action) pair
        qtable = float[state_ct][action_ct]; // np.random.rand(env.stateCount, env.actionCount).tolist()

        for (int i = 0; i < state_ct; ++i) {
            float val = randomf32();
            qtable[i][0] = val;
            qtable[i][1] = 1-val;
        }

        // hyperparameters
        float epochs = 50, gamma = 0.1, epsilon = 0.08, decay = 0.1, reward;
        int done, next_state, action, future_hits;
    }

    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);
        
        auto npos = order_.insert({score_object(min_score_, size, 1), kfs{key, size, 1}});

        map_.insert({key, npos});

        used_ += size;
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it != map_.end()) {
            kfs obj = it->second->second;
            order_.erase(it->second);
            ++obj.hits;

            it.value() = order_.insert({score_object(min_score_, obj.size, obj.hits), obj});
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }

};

#pragma once

#include <stdio.h>
#include <cmath>
#include <map>
#include <vector>
#include "tsl/robin_map.h"

struct gd_object
{
    uint32_t key;
    uint32_t size;
    uint32_t hits;
    float score;
};

struct gds
{
    float operator()(float base, float size, float freq)
    {
        return base + (1 / double(size));
    }
};

struct gdsf
{
    float operator()(float base, float size, float freq)
    {
        return base + (double(freq) / double(size));
    }
};

struct lfuda
{
    float operator()(float base, float size, float freq)
    {
        return base + double(freq);
    }
};

template <typename Score>
class gd_ordered
{
    typedef std::multimap<double, kfs> omap;
    typedef omap::iterator om_it;

    double min_score_;
    
    omap order_;
    tsl::robin_map<uint32_t, om_it> map_;

    Score sc;
    
    void evict(ssize_t size)
    {
        while (size > 0) {
            auto it = order_.begin();
            size -= it->second.size;
            used_ -= it->second.size;
            map_.erase(it->second.key);
            order_.erase(it);
        }
        
        min_score_ = order_.begin()->first;
    }

public:
    gd_cache(size_t capacity):
    cache(capacity),
    min_score_(0)
    {}
    
    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);
        
        map[key] = order_.insert({sc(min_score_, size, 1), kfs{key, size, 1}});
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

            it.value() = order_.insert({sc(min_score_, obj.size, obj.hits), obj});
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }

    void clear()
    {
        map_.clear();
        order_.clear();
        min_score_ = 0;
    }
};

class gd_unordered
{
    typedef tsl::robin_map<uint32_t, uint64_t> ckfs;
    typedef tsl::robin_map<uint32_t, uint64_t> mit;
    typedef std::pair<const double, kfs> ckfs;

    double min_score_;
    
    vector<kfs> order_;
    tsl::robin_map<uint32_t, uint64_t> map_;

    void evict(ssize_t size)
    {
        ssize_t sz = mint(size << 4, capacity_);
        std::vector<kom> candidates;
        candidates.reserve(16384);

        for (auto it = order_.begin(); it != order_.end(); ++it) {
            auto& obj = it->second;
            double fl = score_obj(obj) / obj.size;
            sz -= obj.size;
            candidates.push_back(std::make_pair(fl, it));

            if (sz <= 0)
                break;
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
        
        min_score_ = order_.begin()->first;
    }

public:
    gd_unordered(size_t capacity):
    cache(capacity),
    min_score_(0)
    {}
    
    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);
        
        order_.push_back({score_object(min_score_, size, 1), kfs{key, size, 1}});
        map_[key] = order_.size();
        used_ += size;
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it != map_.end()) {
            kfs& obj = objects_[it->second];
            obj.rescore();
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }

    void clear()
    {
        map_.clear();
        order_.clear();
        min_score_ = 0;
    }
};


#pragma once

#include <stdio.h>
#include <cstdint>
#include <unordered_map>
#include <map>
#include <vector>
#include "cache.hpp"
#include "ska_sort.hpp"
#include "vector.hpp"

#define MAX_K  6

struct lruk_entry
{
    ssize_t x[6];
    uint32_t key;
    uint32_t filesize;
    
    lruk_entry(size_t v, uint32_t tkey, uint32_t tsize):
    key(tkey),
    filesize(tsize)
    {
        for (int i = 0; i < N; ++i)
            x[i] = -1;
        
        x[N - 1] = v;
    }
    
    inline double first()
    {
        if (x[0] != -1) return (x[0]);
        else if (x[1] != -1) return (x[1]) * 0.8;
        else if (x[2] != -1) return (x[2]) * 0.55;
        else if (x[3] != -1) return (x[3]) * 0.21;
        
        return 1;
    }
    
    inline double freq_boost()
    {
        if (x[0] != -1)
            return 10000 / double(x[3] - x[0]);
        
        return 0;
    }
    
    inline double scored()
    {
        float x = freq_boost();
        
        float scorea = (pow(first(), 1) + pow(x,3)) /\
         (::pow(double(filesize), 0.35));
        float scoreb = (pow(first(), 1)) / \
        ((::pow(double(filesize), 0.35)) - pow(x, 0.25));
        
        return scoreb;
    }
    
    void add_request(size_t v, uint32_t K)
    {
        for (int i = 0; i < (K - 1); ++i) x[i] = x[i + 1];
        x[K - 1] = v;
    }
};

class lruk_base 
{
protected:
    uint32_t K_;
    
    lruk_base(uint32_t K):
    K_(K)
    {
        assert(K >= 3);
        assert(K <= MAX_K);
    }
};

class lruk_unordered : protected unordered<lruk_entry>, protected lruk_base
{
public:
    void evict()
    {
        evict([](lruk_entry& ent) { return ent.first(); });
    }

    lruk_vector(uint32_t K):
    lruk_base(K)
    {}
    
    void handle_insert(uint32_t key, uint32_t size, uint32_t counter)
    {
        lruk_entry lk(counter, key, size);
        
        map_.insert({key, vec_.size()});
        vec_.insert({lk.first(), lk});
    }

    void handle_hit(uint32_t idx, uint32_t counter)
    {
        vec_[idx].add_request(timestamp_);
    }
};

class lruk_ordered : protected ordered_policy<lruk_entry>, protected lruk_base
{
public:
    lruk_map(int K):
    lruk_base(K)
    {}

    void evict()
    {
        auto td = list_.back(); 
        map_.erase(td.first);
        size_t size = td.second;

        list_.pop_back();

        return size;
    }
    
    void handle_insert(uint32_t key, uint32_t size, uint32_t counter)
    {
        lruk_entry lk(counter, key, size);

        map_[key] = order_.insert({lk.first(), lk});
    }

    void handle_hit(tree_node<lru_entry>* it)
    {
        lruk lk = it->second->second;
        lk.add_request(timestamp_);
        order_.erase(it->second);

        it->second = order_.insert({lk.first(), lk});
    }
};

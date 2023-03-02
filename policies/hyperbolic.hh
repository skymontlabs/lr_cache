#pragma once

#include <stdio.h>
#include <cmath>
#include <map>
#include <vector>
#include "tsl/robin_map.h"

struct hyperbolic_data
{
    uint32_t key;
    uint32_t size;
    uint32_t hits;
    uint32_t added;

    inline float score(uint32_t counter)
    {
        return float(hits) / float(counter - added);
    }
};

class hyperbolic
{
    void evict(ssize_t size)
    {
        evict();
    }

    void handle_insert(uint32_t key, uint32_t size)
    {
        map_[key] = order_.size();
        order_.insert({counter_, key});
    }
    
    void handle_hit(uint32_t unix, uint32_t key, uint32_t size)
    {
        ++obj.hits;
    }
};


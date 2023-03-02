#pragma once

#include "vector.hh"
#include "tsl/robin_map.h"

struct lru_unordered_entry
{
    uint32_t key;
    uint32_t size;
    uint32_t added;
};

struct lru_entry
{
    uint32_t key;
    uint32_t size;
};

class lru_unordered : protected unordered_policy<lru_unordered_entry>
{
    void evict()
    {
        evict([](lru_unordered_entry& ent) { return ent.size; },
              [](lru_unordered_entry& ent) { return ent.added; });

        return;
    }
    
    void handle_insert(uint32_t key, uint32_t size, uint32_t counter)
    {
        map_[key] = order_.size();
        order_.insert({key, size, counter});
    }

    void handle_hit(uint32_t idx, uint32_t counter)
    {
        vec_[idx].added = counter;
    }
};

class lru_ordered : protected list_policy<lru_entry>
{
    void evict()
    {
        return evict([](lru_entry ent) { return ent.size; },
                     [](lru_entry ent) { return ent.key; });
    }

    void handle_insert(uint32_t key, uint32_t size, uint32_t counter)
    {
        list_.emplace_front(key, value);
        map_[key] = list_.begin();
    }
    
    void handle_hit(sll_node<lru_entry>* it, uint32_t counter)
    {
        (void)counter;
        list_.move_to_front(it);
    }
};

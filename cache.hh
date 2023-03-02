#pragma once

template <typename MappedTo, typename Policy>
class cache
{
    size_t used_;
    size_t capacity_;
    size_t counter_;
    size_t hits_;

    Policy policy_;
    tsl::robin_map<uint32_t, MappedTo> map_;

public:
    cache(Policy& policy):
    used_(0),
    capacity_(0),
    counter_(0),
    hits_(0),
    policy_(policy)
    {}

    void init(size_t capacity)
    {
        used_ = 0;
        capacity_ = capacity;
        counter_ = 0;
        hits_ = 0;
    }

    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it == map_.end()) {
            policy_.handle_hit(it);
            ++hits_;
        } else {
            if (size > capacity_)
                return;

            while (used_ + size > capacity_)
                size -= policy_.evict();
            
            policy_.handle_insert(key, size);
            used_ += size;
        }

        ++counter_;
    }

    void print()
    {
        printf("hit rate: %f\n", double(hits_)/double(counter_));
    }
};

lru_cache = parent_cache<sll_node<lru_entry>*>();

gdsf_cache = parent_cache<tree_node<gdsf_entry>*>(tree_policy<gd_entry>());


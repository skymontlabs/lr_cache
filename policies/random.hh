class random
{
    vector<pair<uint32_t, uint32_t>> vec_;
    tsl::robin_map<uint32_t, uint32_t> map_;

    void evict()
    {
        while (size > 0) {
            auto idx = random32(vec_.size());
            auto td = vec_[idx];

            map_.erase(td.first);      //remove key in map
            used_ -= td.second;
            size -= td.second;
            list_.pop_back();            //remove node in list;

            vec[idx] = vec.back();
            vec.pop_back();
        }
    }

    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);
        
        order_.insert({counter_, key});
        map_.insert({key, order_.size()});
        used_ += size;
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it != map_.end()) {
            vec_[it->second].first = counter_;
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }


};


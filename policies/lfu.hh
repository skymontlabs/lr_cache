
class lfu_unordered
{
    vector<pair<uint32_t, uint32_t>> vec_;
    tsl::robin_map<uint32_t, uint32_t> map_;

    void evict()
    {
    }

    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);
        
        order_.insert({1, key});
        map_.insert({key, order_.size()});
        used_ += size;
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it != map_.end()) {
            ++vec_[it->second].first;
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }
};



class lfu_ordered
{
    list<pair<uint32_t, uint32_t>> list_;
    tsl::robin_map<uint32_t, om_it> map_;
public:
    lfu_ordered(size_t capacity):
    cache(capacity) {}
    
    void evict(ssize_t size)
    {
        while (size > 0) {
            auto td = list_.back(); 
            m_map.erase(td.first);      //remove key in map
            used_ -= td.second;
            size -= td.second;
            list_.pop_back();            //remove node in list;
        }
    }

    void insert(uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_)
            evict(used_ + size - capacity_);

        list_.emplace_front(key, value);  //create new node in list
        m_map[key] = list_.begin();       //create correspondence between key and node
    }
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;
        auto it = map_.find(key);

        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }
};
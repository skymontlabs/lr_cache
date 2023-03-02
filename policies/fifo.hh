
class fifo
{
    typedef std::list<pair<uint32_t, uint32_t>> olist;
    using fifo_iterator = typename olist::const_iterator;

    olist queue_;
    std::unordered_map<uint32_t, fifo_iterator> map_;

    // handle element deletion from a cache
    void evict(ssize_t size)
    {
        while (size > 0) {
            auto rem = queue_.back();
            size -= rem.second;
            used_ -= rem.second;
            map_.erase(rem.first);
            queue_.pop_back();
        }
    }

public:
    fifo();

    void insert(uint32_t key, uint32_t size)
    {
        queue_.emplace_front({key, size});
        map_[key] = queue_.begin();
    }

    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        (void)unix;

        if (map_.find(key) != map_.end()) ++hits_;
        else insert(key, size);

        ++counter_;
    }
};

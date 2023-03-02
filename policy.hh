#pragma once

template <typename Entry>
class unordered_policy
{
protected:
    vector<Entry> vec_;
    tsl::robin_map<uint32_t, uint32_t> map_;

    void replace(uint32_t key, ssize_t size)
    {
        auto it = map_.find(key);
        auto index = it->second;

        // Record stats before removing item
        auto& tag = vec_[index];
        auto size = tag.size;

        // Remove tag for replaced item and update index
        map_.erase(it);
        vec_[index] = vec_.back();
        vec_.pop_back();

        if (index < vec_.size()) {
            map_[vec_[index].key] = index;
        }

        return size;
    }

    template <typename Extract>
    void evict(uint32_t candidates, Extract&& extract_score)
    {
        uint64_t victim = -1;
        double victim_score = std::numeric_limits<double>::max();
        
        for (uint32_t i = 0; i < candidates; ++i) {
            auto idx = rand.next() % vec_.size();
            auto& tag = vec_[idx];
            double score = extract_score(tag);

            if (score < victim_score) {
                victim = idx;
                victim_score = score;
            }
        }

        return replace(tags[victim].id); // where is id
    }
};

template <typename Entry>
class list_policy
{
protected:
    sll<Entry> list_;
    tsl::robin_map<uint32_t, sll_node<Entry>*> map_;

    template <typename ExtractSize, typename ExtractKey>
    void evict(ExtractSize&& extract_size, ExtractKey&& extract_key)
    {
        auto back = order_.back();
        auto size = extract_size(back);
        auto key = extract_key(back);

        map_.erase(key);
        order_.pop_back();

        return size;
    }

};

template <typename Entry>
class tree_policy
{
protected:
    template <typename ExtractSize, typename ExtractKey>
    void evict(ExtractSize&& extract_size, ExtractKey&& extract_key)
    {
        auto back = order_.front();
        auto size = extract_size(back);
        auto key = extract_key(back);

        map_.erase(key);
        order_.pop_back();

        return size;
    }

    tree<Entry> list_;
};


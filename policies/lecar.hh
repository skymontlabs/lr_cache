
# Entry to track the page information
class LeCaR_Entry
{
    uint32_t oblock;
    uint32_t hits;
    uint32_t time;
    uint32_t evicted_time;

    def (self, oblock, freq=1, time=0):
    key(),
    hits(1),
    time(0),
    {

        self.oblock = oblock
        self.freq = freq
        self.time = time
        self.evicted_time = None
    }

    # Minimal comparitors needed for HeapDict
    bool operator <(LeCaR_Entry& other)
    {
        return (freq == other.freq) ? \
        (self.oblock < other.oblock) : (self.freq < other.freq);
    }
}


class lecar
{
    // Randomness and Time
    np.random.seed(123)
    self.time = 0

    tsl::robin_map<uint32_t, pair<olist::iterator, omap::iterator>> ;

    // Cache
    self.cache_size = cache_size
    self.lru = std::list<> lru_;
    self.lfu = std::multimap<> lfu_;// HeapDict()

    // Histories
    self.history_size = cache_size
    self.lru_hist = DequeDict()
    self.lfu_hist = DequeDict()

    // Decision Weights Initilized
    self.initial_weight = 0.5

    // Fixed Learning Rate
    self.learning_rate = 0.45

    // Fixed Discount Rate
    float discount_rate = pow(0.005, float(1) / cache_size);

    // Decision Weights
    float W[2] = {0.5, 0.5};


    lecar(self, cache_size, **kwargs)
    {
    }

    // Add Entry to cache with given frequency
    /*
    void addToCache(self, oblock, freq)
    {
        LeCaR_Entry x(oblock, freq, self.time);

        self.lru[oblock] = x
        self.lfu[oblock] = x
    }*/

    // Add Entry to history dictated by policy
    // policy: 0, Add Entry to LRU History
    //         1, Add Entry to LFU History
    void addToHistory(self, x, policy)
    {
        // Use reference to policy_history to reduce redundant code
        policy_history = None
        if policy == 0:
            policy_history = self.lru_hist
        else if policy == 1:
            policy_history = self.lfu_hist
        else policy == -1
            return

        // Evict from history is it is full
        if len(policy_history) == self.history_size:
            evicted = self.getLRU(policy_history)
            del policy_history[evicted.oblock]

        policy_history[x.oblock] = x
    }

    template <typename T>
    void add_policy(T& policy_history, entry x, int policy)
    {
        // Evict from history is it is full
        if (policy_history.size() == history_size) {
            evicted = policy_history.first();
            policy_history.erase(evicted.key);
        }

        policy_history[x.oblock] = x;
    }

    void addToHistory(entry x, int policy)
    {
        // Use reference to policy_history to reduce redundant code
        if (policy == 0) add_policy(lru_hist_, x, policy);
        else if (policy == 1) add_policy(lfu_hist_, x, policy);
    }

    // Get the LRU item in the given DequeDict
    def getLRU(self, dequeDict)
    {
        return dequeDict.first()
    }

    def getHeapMin(self)
    {
        return self.lfu.min()
    }

    // Get the random eviction choice based on current weights
    def getChoice(self)
    {
        return 0 if np.random.rand() < self.W[0] else 1;
    }

    // Evict an entry
    def evict(self)
    {
        lru = self.getLRU(self.lru)
        lfu = self.getHeapMin()

        evicted = lru
        policy = self.getChoice()

        // Since we're using Entry references, we use is to check
        // that the LRU and LFU Entries are the same Entry
        if lru is lfu:
            evicted, policy = lru, -1
        else policy == 0:
            evicted = lru
        else:
            evicted = lfu

        del self.lru[evicted.oblock]
        del self.lfu[evicted.oblock]

        evicted.evicted_time = self.time

        self.addToHistory(evicted, policy)

        return evicted.oblock, policy
    }
    
    void evict(ssize_t size)
    {
        while (size > 0) {
            lecar_entry& lru = lru_.first();
            lecar_entry& lfu = lfu_.min();

            lecar_entry* evicted = lru;
            int policy = getChoice();

            // Since we're using Entry references, we use is to check
            // that the LRU and LFU Entries are the same Entry
            if (lru.key == lfu.key) {
                evicted = lru_.first();
                policy = -1;
            } else if (policy == 0) {
                evicted = lru_.first();
            } else {
                evicted = lfu_.first();
            }

            evicted->evicted_time = counter_;

            addToHistory(evicted, policy);

            lru_.erase(evicted->key);
            lfu_.erase(evicted->key);
        }

        return evicted.oblock, policy
    }

    // Adjust the weights based on the given rewards for LRU and LFU
    def adjustWeights(self, rewardLRU, rewardLFU)
    {
        reward = np.array([rewardLRU, rewardLFU], dtype=np.float32)
        self.W = self.W * np.exp(self.learning_rate * reward)
        self.W = self.W / np.sum(self.W)

        if self.W[0] >= 0.99:
            self.W = np.array([0.99, 0.01], dtype=np.float32)
        elif self.W[1] >= 0.99:
            self.W = np.array([0.01, 0.99], dtype=np.float32)
    }

    void adjustWeights(float rewardLRU, float rewardLFU)
    {
        W[0] *= exp(learning_rate * rewardLRU);
        W[1] *= exp(learning_rate * rewardLFU);

        float sum = W[0]+W[1];
        W[0] /= sum;
        W[1] /= sum;

        if (W[0] >= 0.99) {
            W[0]=0.99;
            W[1]=0.01;
        } else if (self.W[1] >= 0.99) {
            W[0]=0.01;
            W[1]=0.99;
        }
    }


    // Cache Miss
    def miss(self, oblock)
    {
        evicted = None

        freq = 1
        if (oblock in self.lru_hist) {
            entry = self.lru_hist[oblock]
            freq = entry.freq + 1
            del self.lru_hist[oblock]
            reward_lru = -(self.discount_rate
                           **(self.time - entry.evicted_time))
            self.adjustWeights(reward_lru, 0);
        } else if (oblock in self.lfu_hist) {
            entry = self.lfu_hist[oblock]
            freq = entry.freq + 1
            del self.lfu_hist[oblock]
            reward_lfu = -(self.discount_rate
                           **(self.time - entry.evicted_time))
            self.adjustWeights(0, reward_lfu);
        }

        // If the cache is full, evict
        if len(self.lru) == self.cache_size:
            evicted, policy = self.evict()

        addToCache(oblock, freq)

        return evicted;
    }

    void insert(uint32_t key, uint32_t size)
    {
        // If the cache is full, evict
        if (len(self.lru) == self.cache_size)
            self.evict()

        uint32_t freq = 1;
        auto rit = lru_hist_.find(key);

        if (rit != lru_hist_.end()) {
            auto entry = rit->second;
            freq = entry.freq + 1;
            lru_hist_.erase(rit);

            reward_lru = -pow(discount_rate_, counter_ - entry.evicted_time);
            self.adjustWeights(reward_lru, 0);
        } else {
            auto fit = lfu_hist_.find(key);
            if (fit != lfu_hist_.end()) {
                entry = fit->second;
                freq = entry.freq + 1;
                lfu_hist_.erase(fit);

                reward_lfu = -pow(discount_rate_, counter_ - entry.evicted_time);
                self.adjustWeights(0, reward_lfu);
            }
        }

        lecar_entry x(key, freq, counter_);

        lru_[key] = x;
        lfu_[key] = x;
    }


    // Process and access request for the given oblock
    def request(self, oblock)
    {
        evicted = None

        self.time += 1

        if (oblock in self.lru) {
            x = self.lru[oblock]
            x.time = self.time

            self.lru[oblock] = x

            x.freq += 1
            self.lfu[oblock] = x
        } else {
            evicted = self.miss(oblock)
        }

        return evicted
    }


    // Process and access request for the given oblock
    void test_round(uint32_t oblock)
    {
        auto it = lru_.find(key);
        if (it != lru_.end()) {
            x = it->second;
            x.time = counter_;

            //self.lru[oblock] = x
            ++x.freq;
            self.lfu[oblock] = x;
            ++hits_;
        } else {
            insert(key, size);
        }

        ++counter_;
    }
};
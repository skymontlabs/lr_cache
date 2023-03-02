#pragma once
#include <algorithm>
#include <stdio.h>
#include <cmath>
#include <map>
#include <chrono>
#include <vector>
#include <blaze/Math.h>
#include "tsl/robin_map.h"
//#include "ransac.hh"

#define NOW() std::chrono::steady_clock::now();
#define DURATION(A,B) std::chrono::duration_cast<std::chrono::milliseconds>(B-A).count();

#define VARS 5
#define REFRESH (131072)
#define TRAIN_CUTOFF (REFRESH * 10)

#define score_object(base, size, freq) base + ((double(freq)) / (double(size)))

#define mint(a, b) ((b < a) ? b : a)

struct lro
{
    uint32_t unix;     // 
    uint32_t key;      //
    uint32_t size;     //
    uint32_t hits;     //
    size_t tick;       //

    int32_t hitct[3]; // tick this was added at

    int32_t lru4[4];

    lro(uint32_t u, uint32_t k, uint32_t s, ssize_t t):
    unix(u),
    key(k),
    size(s),
    hits(1),
    tick(t) {
        hitct[0] = -1;
        hitct[1] = -1;
        hitct[2] = 1;

        lru4[0] = -1;
        lru4[1] = -1;
        lru4[2] = -1;
        lru4[3] = t;
    }

    double lru_data() const
    {
        if (lru4[0] != -1) return (lru4[0]);
        else if (lru4[1] != -1) return (lru4[1]) * 0.8;
        else if (lru4[2] != -1) return (lru4[2]) * 0.55;
        else if (lru4[3] != -1) return (lru4[3]) * 0.21;
        
        return 1;
    }

    void cycle_bins()
    {
        hitct[0] = hitct[1];
        hitct[1] = hitct[2];
        hitct[2] = 0;
    }

    void add_hit(size_t ts)
    {
        ++hits;
        ++hitct[2];

        lru4[0] = lru4[1];
        lru4[1] = lru4[2];
        lru4[2] = lru4[3];
        lru4[3] = ts;
    }

    std::string toString() const
    {
        return std::to_string(unix) + " " +  std::to_string(key) + " " +  \
        std::to_string(size) + " " +  std::to_string(hits) + " " +  std::to_string(tick) + " " + \
        std::to_string((int)hitct[0]) + " " +  std::to_string((int)hitct[1]) + " " +  std::to_string((int)hitct[2]);
    }
};

class lr_cache
{
    typedef std::multimap<double, lro> omap;
    typedef omap::iterator om_it;
    typedef std::pair<double, om_it> kom;
    typedef std::pair<const double, lro> clro;

    size_t used_;
    size_t capacity_;
    size_t timestamp_;
    size_t unix_;

    size_t hits_;

    double min_score_;
    
    omap order_;
    tsl::robin_map<uint32_t, om_it> map_;


    double vars[5];

    //timer_results& res_;
    /**/
    void train(blaze::DynamicMatrix<double>& X, blaze::DynamicMatrix<double>& y)
    {
        blaze::DynamicMatrix<double> Xt(X);
        Xt.transpose();

        blaze::DynamicMatrix<double> XtX = Xt * X;
        blaze::DynamicMatrix<double> Xty = Xt * y;
        invert(XtX);
        
        blaze::DynamicMatrix<double> out = XtX * Xty;
        vars[0] = out(0,0);
        vars[1] = out(1,0);
        vars[2] = out(2,0);
        vars[3] = out(3,0);
        vars[4] = out(4,0);
    }
        
    void train_linear()
    {
        auto begin = NOW()
        blaze::DynamicMatrix<double> X(order_.size(), 5, 0);
        blaze::DynamicMatrix<double> y(order_.size(), 1, 0);
        uint32_t idx = 0;
        
        for (const clro& dc : order_) {
            const lro& obj = dc.second;

            //std::cout << obj.toString() << '\n';

            if (obj.hitct[0] >= 0 && obj.hitct[1] >= 0 && obj.hitct[2] >= 0) {
                X(idx, 0) = 1;
                // only frequency
                X(idx, 1) = sqrt(double(obj.hits));//double(obj.hits) / double(timestamp_ - obj.tick);//
                // only recency
                X(idx, 2) = obj.lru_data() / double(timestamp_); 
                // scope 1 recency
                X(idx, 3) = double(obj.hitct[1]) / double(obj.hits);
                // scope 2 recency
                X(idx, 4) = double(obj.hitct[1] + obj.hitct[0]) / double(obj.hits);

                y(idx, 0) = log2(obj.hitct[2] + 1);
                ++idx;
            }
        }

        X.resize(idx, 5);
        y.resize(idx, 1);
        X.shrinkToFit();
        y.shrinkToFit();

        train(X, y);
        auto end = NOW()
        auto dur = DURATION(begin,end)
        printf("runtime time elapsed: %lld ms\n",dur);
    }

    double score_obj(lro& obj)
    {
        double s =
        vars[0] + \
        vars[1] * sqrt(double(obj.hits)) + \
        vars[2] * (obj.lru_data() / double(timestamp_)) + \
        vars[3] * (double(obj.hitct[1]) / double(obj.hits)) + \
        vars[4] * (double(obj.hitct[1] + obj.hitct[0]) / double(obj.hits));

        return s;
    }

    void evict_easy(ssize_t size)
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


    void evict(ssize_t size)
    {
        while (size > 0) {
            auto it = order_.begin();
            auto& obj = it->second;
            size -= obj.size;
            used_ -= obj.size;
            map_.erase(obj.key);
            order_.erase(it);
        }
        
        min_score_ = order_.begin()->first;
    }
    
    void insert(uint32_t unix, uint32_t key, uint32_t size)
    {
        if (size > capacity_)
            return;

        if (used_ + size > capacity_) {
            if (vars[0] == -100000) evict(used_ + size - capacity_);
            else evict_easy(used_ + size - capacity_);
        }

        auto npos = order_.insert({score_object(min_score_, size, 1), lro(unix, key, size, timestamp_)});
        map_.insert({key, npos});
        
        //auto begin = NOW()
        //map_.insert({key, order_.begin()});
        //auto end = NOW()
        //auto tset = DURATION(begin, end)
        //res_.add_s(tset);
        used_ += size;
    }

public:
    lr_cache(size_t capacity)://, timer_results& res
    used_(0),
    capacity_(capacity),
    timestamp_(0),
    hits_(0),
    min_score_(0)
    {
        vars[0] = -100000;
    }
    //res_(res)
    
    void test_round(uint32_t unix, uint32_t key, uint32_t size)
    {
        //auto begin = NOW()
        auto it = map_.find(key);
        //printf("%zu\n", map_.size());
        //auto end = NOW()
        //auto tget = DURATION(begin, end)

        if (it != map_.end()) {
            lro obj = it->second->second;
            order_.erase(it->second);
            obj.add_hit(timestamp_);

            it.value() = order_.insert({score_object(min_score_, obj.size, obj.hits), obj});
            //////////res_.add_f(tget);
            ++hits_;
        } else {
            insert(unix, key, size);
            //res_.add_xf(tget);
        }

        if (((timestamp_ + 1) % REFRESH) == 0) {
            //puts("train");
            if ((timestamp_ + 1) >= TRAIN_CUTOFF)
                train_linear();
                
            std::vector<om_it> to_erase;
            for (auto itr = order_.begin(); itr != order_.end(); ++itr) {
                if (itr->second.lru_data() == -1) { to_erase.push_back(itr); }//puts("erased");
                else { itr->second.cycle_bins(); }//puts("cycle_binsd"); 
            }

            for (auto e : to_erase) {
                order_.erase(e);
            }
        }

        ++timestamp_;
        unix_ = (unix > unix_) ? unix : unix_;
    }


    void clear()
    {
        map_.clear();
        order_.clear();
        min_score_ = 0;
    }

    void print()
    {
        for (double v : vars) {
            printf("%f ", v);
        }
        puts("");
        printf("hit rate: %f\n", double(hits_)/double(timestamp_));
    }

};


#pragma once

#include <cstdlib>
#include <cmath>

#define PHI 1.618033988749894848204586834

template <typename V>
class vector
{
    V* arr_;
    
    size_t cur_;
    size_t end_;
    
    void reallocate()
    {
        size_t new_size = static_cast<size_t>(ceil(double(end_) * PHI));
        
        V* new_arr = (V*)realloc(arr_, new_size * sizeof(V));
        
        arr_ = new_arr;
        end_ = new_size;
    }
    
public:
    vector(size_t initial):
    cur_(0),
    end_(initial)
    {
        arr_ = (V*)malloc(initial * sizeof(V));
    }
    
    inline void replace(size_t idx)
    {
        arr_[idx] = arr_[cur_ - 1];
        pop_back();
    }
    
    inline void pop_back()
    {
        cur_ -= (cur_ > 0);
    }
    
    inline void push_back(V value)
    {
        if (cur_ == end_)
            reallocate();
        
        arr_[cur_++] = value;
    }
    
    inline void push_back_nv()
    {
        if (cur_ == end_)
            reallocate();
        
        cur_++;
    }
    
    inline V* alloc_back()
    {
        if (cur_ == end_)
            reallocate();
        
        auto back = &arr_[cur_];
        cur_++;

        return back; 
    }
    
    inline V& operator[](size_t idx)
    {
        return arr_[idx];
    }
    
    inline V& back()
    {
        return arr_[cur_ - 1];
    }
    
    inline V* back_ptr()
    {
        return &arr_[cur_ - 1];
    }
    
    inline size_t size()
    {
        return cur_;
    }
    
    inline void clear()
    {
        cur_ = 0;
    }
};

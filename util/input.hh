#pragma once

#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class input
{
    uint8_t* ptr_;

    int fd_;
    int owns_ptr_;

    size_t block_size_;
    size_t total_size_;
    size_t offset_size_;

public:
    inline input():
    ptr_(nullptr),
    fd_(-1),
    owns_ptr_(1),
    block_size_(0),
    total_size_(0),
    offset_size_(0) {}

    inline input(uint8_t* ptr, size_t size):
    ptr_(ptr),
    fd_(-1),
    owns_ptr_(0),
    block_size_(size),
    total_size_(0),
    offset_size_(0) {}
    
    inline ~input()
    {
        if (fd_ != -1)
            close(fd_);
        
        if (ptr_ && owns_ptr_)
            delete[] ptr_;
    }

    inline uint8_t* data()
    {
        return ptr_;
    }
    
    int init(const char* fn, size_t size=0)
    {
        int fd = open(fn, O_RDONLY, O_NDELAY);
        if (fd == -1)
            return -1;
        
        struct stat st;
        if (fstat(fd, &st) == -1)
            return -1;
        
        total_size_ = st.st_size;
        
        fd_ = fd;

        return 0;
    }
    
    ssize_t read_section()
    {
        size_t read_size = ((offset_size_ + block_size_) < total_size_) ? block_size_ : (total_size_ - offset_size_);
        ssize_t rr = ::read(fd_, ptr_, read_size);
        if (rr == -1)
            return -1;
        
        offset_size_ += read_size;
        
        return read_size;
    }
    
    bool has_next()
    {
        return total_size_ > offset_size_;
    }
};

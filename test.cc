#include <stdio.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <blaze/Math.h>
#include "lrc.hh"
//#include "gdsf.hh"
#include "input.hh"

#define READ_BYTES 12UL * (1UL << 27)

template <typename CacheT>
void test_cache_fi(CacheT& cached, const char* fi)
{
    uint8_t* data = (uint8_t*)malloc(READ_BYTES);
    input file_handler(data, READ_BYTES);

    if (file_handler.init(fi) != 0) {
        puts("failed read");
        exit(-1);
    }

    uint32_t unix, key, size, reads=0;

    auto begin = NOW()
    
    while (file_handler.has_next()) {
        uint64_t size = file_handler.read_section();
        uint8_t* p = data;
        uint64_t i = 0;

        if (reads == 0) {
            ubits = *(uint32_t*)p;
            kbits = (ubits >> 6) & 0x3F;
            sbits = ubits & 0x3F;
            ubits >>= 12;
            p += 4;
        }

        while (i < size) {
            if (ContainUnix) {
                uint32_t unix = bb.read(ubits);
                uint32_t key  = bb.read(kbits);
                uint32_t size = bb.read(sbits);

                cached.test_round(unix, key, size);
            } else {
                uint32_t key  = bb.read(kbits);
                uint32_t size = bb.read(sbits);

                cached.test_round(0, key, size);
            }
        }

        bb.reset();
        ++reads;
    }
    
    auto end = NOW()
    auto dur = DURATION(begin,end)
    auto min = dur / (60 * 1000);
    auto mil = dur % (60 * 1000);
    printf("time elapsed: %lld.%f min\n", min, double(mil) / (60 * 1000));

    free(data);
}

void* test_gdsf_cache(void *filepath)
{
    gdsf_cache c(1 << 30);
    test_cache_file(c, (const char*)filepath);

    c.print();
    return NULL;
}

void* test_lruk_cache(void *filepath)
{
    lruk_cache c(1 << 30);
    test_cache_file(c, (const char*)filepath);

    c.print();
    return NULL;
}

void* test_lr_cache(void *filepath)
{
    lr_cache c(1 << 30);
    test_cache_file(c, (const char*)filepath);

    c.print();
    return NULL;
}

void* test_gboost_cache(void *filepath)
{
    lr_cache c(1 << 30);
    test_cache_file(c, (const char*)filepath);

    c.print();
    return NULL;
}

int main(int argc, const char* argv[])
{
    pthread_t t0,t1,t2,t3,t4;

    if (pthread_create(&t0, NULL, test_cache, (void*)"wiki2018.trb") != 0) {
        printf(">>> unable to create background thread\n");
        fflush(stdout);
        return 1;
    }

    if (pthread_create(&t1, NULL, test_cache, (void*)"wiki2019.trb") != 0) {
        printf(">>> unable to create background thread\n");
        fflush(stdout);
        return 1;
    }

    if (pthread_create(&t2, NULL, test_cache, (void*)"wiki2019.trb") != 0) {
        printf(">>> unable to create background thread\n");
        fflush(stdout);
        return 1;
    }

    if (pthread_create(&t3, NULL, test_cache, (void*)"wiki2019.trb") != 0) {
        printf(">>> unable to create background thread\n");
        fflush(stdout);
        return 1;
    }

    if (pthread_create(&t4, NULL, test_cache, (void*)"wiki2019.trb") != 0) {
        printf(">>> unable to create background thread\n");
        fflush(stdout);
        return 1;
    }

    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    return 0;
}
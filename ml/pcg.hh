
/**
BEGIN minimalist PCG code
*/

typedef struct pcg_state_setseq_64 {    // Internals are *Private*.
    uint64_t state;             // RNG state.  All values are possible.
    uint64_t inc;               // Controls which RNG sequence (stream) is selected. Must *always* be odd.
} pcg32_random_t;

static pcg32_random_t pcg32_global = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };

static inline uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}


__attribute__((always_inline))
static inline uint32_t pcg32_random() {
    return pcg32_random_r(&pcg32_global);
}

// map random value to [0,range) with slight bias
static inline uint32_t pcg32_random_bounded_divisionless_with_slight_bias(uint32_t range)
{
    uint64_t random32bit, multiresult;
    random32bit =  pcg32_random();
    multiresult = random32bit * range;
    return multiresult >> 32; // [0, range)
}

// good old Fisher-Yates shuffle, shuffling an array of integers, without division
static inline void shuffle_pcg_divisionless_with_slight_bias(uint32_t *storage, uint32_t size) {
    uint32_t i;
    for (i=size; i>1; i--) {
        uint32_t nextpos = pcg32_random_bounded_divisionless_with_slight_bias(i);
        uint32_t tmp = storage[i-1];// likely in cache
        uint32_t val = storage[nextpos]; // could be costly
        storage[i - 1] = val;
        storage[nextpos] = tmp; // you might have to read this store later
    }
}

/*
* Shuffles a sequence
*/

static inline void permute(uint32_t* storage, uint32_t size)
{
    for (int i = 0; i < size; ++i)
        storage[i] = i;
    
    shuffle_pcg_divisionless_with_slight_bias(storage, size);
}

/*
*   Copies out all the input
*/
static inline void indexed_copy(float* __restrict out_a, float* __restrict out_b,
    const float* __restrict input_a, const float* __restrict input_b,
    const uint32_t* __restrict idxs, uint32_t rows, uint32_t cols)
{
    for (int i = 0; i < rows; ++i) {
        uint32_t r = idxs[i] * cols;
        uint32_t o = i * cols;

        memcpy(&out_a[o], &input_a[r], cols * sizeof(float));
        out_b[i] = input_b[idxs[i]];
    }
}

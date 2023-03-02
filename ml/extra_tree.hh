#include <iostream>
#include <cmath>


template <typename ModelType>
class extra_tree
{
    // create decision tree
    ModelType* model_;
    int model_ct_;
    
public:
    extra_tree(int model_ct):
    model_ct_(model_ct)
    {}

    void calc(float* X, float* y, float* w, float lr, uint32_t samples, uint32_t features)
    {
        // randomly split features

        // 
    }
};

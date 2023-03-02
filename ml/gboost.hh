#include <iostream>
#include <cmath>


template <typename ModelType>
class gboost
{
    // create decision tree
    ModelType* model_;
    
public:
    void calc(float* X_train, float* y_train, float* w, float lr,
        uint32_t samples, uint32_t features, uint32_t rounds)
    {
        uint32_t max_depth_ = 3;
        // transposed x values
        float* X_t = new float[samples * features * max_depth_];
        // y residuals (y - mean)
        float* residuals = new float[samples];
        // residual predictions
        float* predicted = new float[samples];
        // flags for node splitting
        uint64_t* flags = new uint64_t[((samples + 7) * (max_depth_ - 1) >> 2)];

        double avg = 0;
        
        transpose(X_t, X, samples, features);
        
        // calculate average of y
        for (int i = 0; i < samples; ++i)
            avg += y_train[i] * w[i];
        
        // calculate residuals
        for (int i = 0; i < samples; ++i)
            residuals[i] = y_train[i] - avg;
        
        

        // train decision tree
        for (int i = 0; i < rounds; ++i) {
            //
            model_[i].train(predicted, X_t, residuals, w, samples, features);

            //residuals = individual Y - (avg + (predicted * lr));
            for (int i = 0; i < samples; ++i) {
                //residuals[i] = y[i] - (avg + predicted[i] * lr);
                //residuals[i] = y[i] - avg - predicted[i] * lr;
                residuals[i] -= predicted[i] * lr;
            }
        }
    }

    void validate(float* X, float* y)
    {

    }
};

#pragma once

#include <stdio.h>
#include <stdint.h>
//#include <blaze/Blaze.h>
#include <iostream>
#include <random>
#include <algorithm>

#define NUM_FEATURES 5

template <typename Model, typename Loss, typename Metric>
class RANSAC
{
    uint32_t n_;              // `n`: Minimum number of data points to estimate parameters
    uint32_t k_;              // `k`: Maximum iterations allowed
    float t_;                 // `t`: Threshold value to determine if points are fit well
    uint32_t d_;              // `d`: Number of close data points required to assert model fits well
    
    Model&& model_;           // `model`: class implementing `fit` and `predict`
    Loss&& loss_;             // `loss`: function of `y_true` and `y_pred` that returns a vector
    Metric&& metric_;         // `metric`: function of `y_true` and `y_pred` and returns a float
    Model* best_fit_;
    Model* maybe_model_;
    Model* better_model_;
    float best_error_;

    RANSAC(Model&& model, Loss&& loss, Metric&& metric,
        uint32_t n=10, uint32_t k=100, float t=0.05, uint32_t d=10):
    model_(model),
    loss_(loss),
    metric_(metric),
    n_(n),
    k_(k),
    t_(t),
    d_(d) {}

    #define X_IDX(idx) (NUM_FEATURES * idx)
    #define Y_IDX(idx) (idx)

    void fit(float* X, float* y, int samples)
    {
        int test_size = samples - n_;
        int inlier_size = 0;

        uint32_t ids[samples];

        float* shuffled = new float[(samples + n_) * NUM_FEATURES + (samples * 3)];

        float* Xshuffled = shuffled;//new float[(samples + n_) * NUM_FEATURES];
        float* Yshuffled = shuffled + (samples + n_) * NUM_FEATURES;//new float[(samples * 3)];

        float* Xn = &Xshuffled[X_IDX(n_)];
        float* Yn = &Yshuffled[Y_IDX(n_)];
        float* Xend = &Xshuffled[X_IDX(samples)];
        float* Yend = &Yshuffled[Y_IDX(samples)];

        float* Ypred = &Yshuffled[Y_IDX(samples + n_)];

        // set ids into array to be later shuffled
        for (int i = 0; i < samples; ++i)
            ids[i] = i;

        for (int i = 0; i < k_; ++i) {
            // shuffle ids
            permute(ids, samples);
            // copy over the X and Y values in the shuffled order
            indexed_copy(Xshuffled, Yshuffled, X, y, ids, samples, NUM_FEATURES);

            maybe_model_->copy_model(model_);
            maybe_model_->fit(Xshuffled, Yshuffled, n_);

            memcpy(Xend, Xn, NUM_FEATURES * test_size * sizeof(float));
            memcpy(Yend, Yn, test_size * sizeof(float));

            inlier_size = 0;
            maybe_model_->predict(Ypred, Xend, test_size);

            for (int i = 0; i < test_size; ++i) {
                if (loss_(Ypred[i], Yend[i]) < t_) {
                    memcpy(&Xn[X_IDX(inlier_size)], &Xend[X_IDX(inlier_size)], sizeof(float) * NUM_FEATURES);
                    Yn[Y_IDX(inlier_size)] = Yend[i];

                    ++inlier_size;
                }
            }
            
            // if 
            if (inlier_size > d_) {
                better_model_->copy_model(model_);
                better_model_->fit(Xshuffled, Yshuffled, n_ + inlier_size);

                // predict the rest
                auto results = &Yshuffled[Y_IDX(samples << 1)];
                better_model_->predict(results,
                                      &Xshuffled[X_IDX(n_ + inlier_size)],
                                      test_size - inlier_size);

                // predict whether or not
                float this_error = metric_(&Yshuffled[inlier_points], results, test_size - inlier_size);
                if (this_error < best_error_) {
                    best_error_ = this_error;
                    best_fit_ = maybe_model_;
                }
            }
        }
    }

    float predict(float* X)
    {
        return best_fit_->predict(X);
    }
};

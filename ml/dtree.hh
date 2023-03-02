#include "types.hh"

struct tree_node
{
    float values[2];
    uint8_t feature;
    uint8_t leaf;
};

static inline float mse(float yh, float y)
{
    float diff = yh - y;
    return diff * diff;
};


void transpose(float* out, const float* in, u32 samples, u32 features)
{
    float* out_ptrs[features];
    for (int j = 0; j < features; ++j)
        out_ptrs[j] = &out[samples * j];

    for (int i = 0; i < samples; ++i) {
        for (int j = 0; j < features; ++j)
            out_ptrs[j][i] = *in++;
    }
}

#define FLAG_CT(x) (x)
#define PARENT(x) (((x - 1) >> 1))
#define LEFT_CHILD(x) ((i << 1) + 1)
#define RIGHT_CHILD(x) ((i + 1) << 1)
#define BETWEEN(val,lo,hi) (val >= lo && val <= hi)

class decision_tree_internal
{
    tree_node* root_;
    float* begin_;
    float* cur_;

    u32 max_depth_;
    u32 min_leaf_samples_;
    
    void _split(bool has_children, bool penultimate, float** x_splits, float** y_splits, float** w_splits,
                u64* flag, float* unique_values, u32 depth, float lo_bound, float hi_bound,
                u32 i, u32 fea, u32 samples)
    {

        float* x = x_splits[i],* y = y_splits[i],* w = w_splits[i];
        
        float delta_best = -1000000.0f, delta = 0, mse_l = 0, mse_r = 0;
        u32 unique_ct = 0, l_size = 0, l_size_best = 0;
        unique_ct = unique(unique_values, x, samples, fea);

        for (int fn = 0; fn < fea; ++fn) {
            for (int j = 0; j < unique_ct; ++j) {
                lo_bound = unique_values[j];

                delta = _cost(flag + FLAG_CT(samples), x + fn * samples, y, w, lo_bound, hi_bound, samples, l_size);

                if (delta_best < delta) {
                    delta_best = delta;
                    l_size_best = l_size;
                    
                    memcpy(flag, flag + FLAG_CT(samples), samples);
                }
            }
        }

        add_branch(has_children, x_splits, y_splits, w_splits, x, y, w, fl, lo_bound, hi_bound, mse_l, mse_r, i, fea, samples, l_size_best);
        
    }

    void copy_idx(float* x_l, float* x_r, float* y_l,
                  float* y_r, float* w_l, float* w_r,
                  const float* x, const float* y, const float* w,
                  const u64* fl, u32 samples, u32 fea)
    {
        u64 flag = fl[0];
        u32 l_i = 0, r_i = 0;

        for (int i = 0; i < fea; ++i) {
            for (int j = 0; j < samples; ++j) {
                if (flag & (1 << j)) {
                    x_l[l_i] = x[j];
                    y_l[l_i] = y[j];
                    w_l[l_i] = w[j];
                    ++l_i;
                } else {
                    x_r[r_i] = x[j];
                    y_r[r_i] = y[j];
                    w_r[r_i] = w[j];
                    ++r_i;
                }

                if ((j & 63) == 0 && j != 0)
                    flag = fl[j >> 6];
            }
            
            flag = fl[0];
        }
    }
    
    
    template <int left>
    float copy_half_idx(float* x_h, float* y_h, float* w_h,
                  const float* x, const float* y, const float* w,
                  const u64* fl, u32 samples, u32 fea)
    {
        float avg = 0;
        u64 flag = fl[0];
        u32 h_i = 0;

        for (int i = 0; i < fea; ++i) {
            for (int j = 0; j < samples; ++j) {
                if (left == 0) {
                    if (flag & (1 << j)) {
                        avg += y[j] * w[j];
                    } else {
                        x_h[h_i] = x[j];
                        y_h[h_i] = y[j];
                        w_h[h_i] = w[j];
                        ++h_i;
                    }
                } else {
                    if (flag & (1 << j)) {
                        x_h[h_i] = x[j];
                        y_h[h_i] = y[j];
                        w_h[h_i] = w[j];
                        ++h_i;
                    } else {
                        avg += y[j] * w[j];
                    }
                }

                if ((j & 63) == 0 && j != 0)
                    flag = fl[j >> 6];
            }
            
            flag = fl[0];
        }
        
        return avg;
    }
    
    /*
    template <int left>
    float copy_half_idx(float* out, const float* y, const u64* fl, u32 samples, u32 fea)
    {
        float avg = 0;
        u64 flag = fl[0];
        u32 l_i = 0, r_i = 0;

        for (int i = 0; i < fea; ++i) {
            for (int j = 0; j < samples; ++j) {
                if (left == 0) {
                    if (flag & (1 << j)) avg += y[j]; // left[l_i++] = y[j];
                    else out[r_i++] = y[j]; // avg += y[j];
                } else if (left == 1) {
                    if (flag & (1 << j)) avg += y[j]; // left[l_i++] = y[j];
                    else out[r_i++] = y[j]; // avg += y[j];
                } else {
                    if (flag & (1 << j)) out[l_i++] = y[j];
                }

                if ((j & 63) == 0 && j != 0) flag = fl[j >> 6];
            }
            
            flag = fl[0];
        }
        
        return avg;
    }*/
    
    
    u32 unique(float* out, const float* in, u32 samples, u32 fea)
    {
        float* o = out;
        float buffer[32];
        memcpy(buffer, in, samples * fea * sizeof(float));

        std::sort(buffer, buffer + samples);

        for (int i = 1; i < samples; ++i) {
            if (buffer[i - 1] != buffer[i])
                *o++ = buffer[i];
        }
        
        return u32(o - out);
    }
    
    // calculate mean for creating 2 leaf nodes instead of creating branch nodes
    std::pair<float, float> mean(const float* y, const float* weight, const u64* fl, u32 samples)
    {
        u64 flag = 0;
        float m_l = 0, m_r = 0, w_l = 0, w_r = 0;

        for (int i = 0; i < samples; ++i) {
            if (flag & (1 << i)) {
                m_l  += y[i] * weight[i];
                w_l += weight[i];
            } else {
                m_r  += y[i] * weight[i];
                w_r += weight[i];
            }

            if ((i & 63) == 0 && i != 0) {
                flag = fl[i >> 6];
            }
        }
        
        return std::pair<float, float>(m_l / w_l, m_r / w_r);
    }
    

    //delta = _cost(y, w) - (len(l) * _cost(y[l], w[l]) + len(r) * _cost(y[r], w[r])) / len(y)
    void add_branch(bool has_children, float** x_splits, float** y_splits, float** w_splits,
                    float* x, float* y, float* w, const u64* fl, float lo_bound, float hi_bound,
                    float mse_l, float mse_r, u32 i, u32 fea, u32 samples, u32 l_samples)
    {
        // do we know the status of whether some are premature leaves so we can save space?
        u32 r_samples = samples - l_samples;
        
        float* x_lc = allocate(l_samples * fea * sizeof(float));
        float* x_rc = allocate(r_samples * fea * sizeof(float));
        float* y_lc = allocate(l_samples * sizeof(float)); // y + samples;
        float* y_rc = allocate(r_samples * sizeof(float)); // ;y_lc + l_samples;
        float* w_lc = allocate(l_samples * sizeof(float)); // ;y_lc + l_samples;
        float* w_rc = allocate(r_samples * sizeof(float)); // ;y_lc + l_samples;
        
        root_[i].leaf      = 0;
        root_[i].feature   = fea;
        root_[i].values[0] = lo_bound;
        root_[i].values[1] = hi_bound;

        u32 l_idx = LEFT_CHILD(i);
        u32 r_idx = RIGHT_CHILD(i);
        
        if (has_children || (mse_l < 4 && mse_r < 4)) { // 2 children are leaves
            std::pair<float, float> mv = mean(y, w, fl, samples);
            
            root_[l_idx].leaf      = 1;
            root_[l_idx].values[0] = mv.first;
            
            root_[r_idx].leaf      = 1;
            root_[r_idx].values[0] = mv.second;
        } else if (mse_l < 4) { // 1 child leaf, another not
            float mean_l = copy_half_idx<0>(x_rc, y_rc, w_rc, x, y, w, fl, samples, fea);
            
            root_[l_idx].leaf      = 1;
            root_[l_idx].values[0] = mean_l;
        } else if (mse_r < 6) { // 1 child leaf, another not
            float mean_r = copy_half_idx<1>(x_lc, y_lc, w_lc, x, y, w, fl, samples, fea);

            root_[r_idx].leaf      = 1;
            root_[r_idx].values[0] = mean_r;
        } else { // 2 children stems
            x_lc = allocate(l_samples * fea * sizeof(float));
            x_rc = allocate(r_samples * fea * sizeof(float));
            
            copy_idx(x_lc, x_rc, y_lc, y_rc, w_lc, w_rc, x, y, w, fl, samples, fea);
        }
        
        x_splits[l_idx] = x_lc;
        y_splits[l_idx] = y_lc;
        w_splits[l_idx] = w_lc;
        x_splits[r_idx] = x_rc;
        y_splits[r_idx] = y_rc;
        w_splits[r_idx] = w_rc;
    }
    

    float _cost(u64* idxs, float* x, float* y, float* w,
                float lo_bound, float hi_bound, u32 samples, u32& l_samples)
    {
        u64* idx = idxs;
        u64 flag = 0;

        float mul = 0,
              avg = 0,
              avg_l = 0,
              avg_r = 0,
              sum_wl = 0,
              sum_wr = 0,
              diff_total = 0,
              diff_sep = 0,
              cost_total = 0,
              cost_sep = 0;

        // calculate averages for groups
        for (int i = 0; i < samples; ++i) {
            mul = y[i] * w[i];

            //if (BETWEEN(x[i],lo_bound,hi_bound)) {
            if (x[i] > lo_bound) {
                avg_l += mul;
                flag  |= (1 << (i & 63));
            } else {
                avg_r += mul;
            }

            //if ((i  & 63) == 0 && i != 0) {
            if (((i + 1) & 63) == 0) {
                //*idx++ = flag;
                idxs[(i + 1) >> 6] = flag;
                flag = 0;
            }
        }

        *idx++ = flag;

        avg = avg_l + avg_r;
        avg_l /= sum_wl;
        avg_r /= sum_wr;

        // calculate differences
        for (int i = 0; i < samples; ++i) {
            diff_total  = (y[i] - avg);
            diff_sep   = y[i] - (BETWEEN(x[i],lo_bound,hi_bound) ? avg_l : avg_r);

            cost_total += diff_total * diff_total;
            cost_sep   += diff_sep * diff_sep;
        }


        return (cost_total - cost_sep) / samples;
        //return cost_total - (cost_l + cost_r) / samples;
        //return cost / samples;
        //return l_size * (_cost(y, w, l)) + r_size * _cost(y, w, r);
        //delta = _cost_uw(y, w) - (count.first * _cost(y, w, l) + count.second * _cost(y, w, r)) / label_ct;
    }



    
  
    inline float* allocate(u32 size)
    {
        float* cur = cur_;
        cur_ += size;
        
        return cur;
    }
    
    
public:
    
    decision_tree_internal(tree_node* root, float* begin, u32 max_depth=3, u32 min_leaf_samples=1):
    root_(root),
    begin_(begin),
    min_leaf_samples_(min_leaf_samples) {}
    
    
    
    void train(float* out, float* x_transpose, float* y, float* w, u32 samples, u32 features)
    {
        u64 parentleaf = 0;
        u32 max_nodes = (1 << max_depth_) - 1, max_branches = 4, left_idx, right_idx, depth = 0;
        
        float* uniques = new float[samples * features];
        float* val_buffer = (float*)malloc(samples * (max_depth_ - 1) * sizeof(float));
        float* unique_values = (float*)malloc(samples * features * (max_depth_ - 1) * sizeof(float));
        u64* flags = (u64*)malloc((samples + 7) * (max_depth_ - 1) >> 3);
        u32 l_size, r_size;
        
        float* x_splits[max_nodes];
        float* y_splits[max_nodes];
        float* w_splits[max_nodes];
        u32 sizes[max_nodes];

        sizes[0] = samples;
        
        x_splits[0] = x_transpose;
        x_splits[1] = x_transpose + samples * features;
        x_splits[3] = x_transpose + samples * features * 2;
        
        y_splits[0] = y;
        w_splits[0] = w;
        

        int i = 0;
        
        //x_transpose
        //[root: x_transpose][l1: sorted values for 2 different nodes][l2: ][l3: last layer, so just mean]
        
        //   0
        // 1   2
        //3 4 5 6
        
        do {
            depth = log2(i + 1);

            if ((parentleaf & (1 << PARENT(i))) == 0) {
                left_idx = LEFT_CHILD(i);
                right_idx = RIGHT_CHILD(i);
                
                // returns the left child size
                std::pair<u32, u32> div = split(uniques, x_splits[i], y_splits, w_splits, sizes, i, left_idx, right_idx, sizes[i]);
                
                sizes[left_idx] = div.first;
                sizes[right_idx] = div.second;
            }
            
        } while (i < max_branches);
    }
};

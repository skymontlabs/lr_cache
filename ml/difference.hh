/*
Kendall tau correlating the feature individually with y

STD and MAD of the y values of each category, trying to minimize both,
    also trying to minimize the worse one, or trying to minimize the better one (6)

Median deviation X

Mean (pow1 and pow2) deviation X

Spearman R (Xi value and y value in each category or total)
Pearson R 

Z test (between the distribution of Y values)

KS test

Can choose all or just some (randomized)

*/


// sort first
//

class dtree
{
    tree_node* root_;
    float* begin_;
    float* cur_;

    u32 max_depth_;
    u32 min_node_samples_;
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

struct node
{
    int feature_idx;
    int samples;
    int breakpoint;
};

void split_node(float* x, float* y, float* w, u32 samples, u32 features)
{
    float lvl, best_lvl = FLOAT_MAX;
    int breakpoint, k=0;

    for (int j = 0; j < features; ++j) {
        
        int k = 0;

        while (k < samples) {
            while (k < samples&&x[k]==x[k+1])++k;

            lo_bound = values[k];
            lvl = score(x, y, w, k);

            if (best_lvl < lvl) {
                best_lvl = lvl;
                breakpoint = k;
            }

            ++k;
        }

        x += samples;
    }
}

void train(float* x, float* y, float* w, u32 samples, u32 features)
{
    transpose(x, );

    // create the indices and sort the x values
    pair<uint32_t,float> sort_idxs[features][samples];

    for (int i = 0; i < samples; ++i)
        sort_idxs[i] = {i,xti[i]};

    sort(sort_idxs,sort_idxs+samples,
    [](pair<uint32_t,float>& a, pair<uint32_t,float>& b)
    { return a.second < b.second; });


    float xt_mean[samples];
    memset(xt_mean,0,samples*sizeof(u32));


    node node_array[(1<<max_height)-1];


    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < NODE_CT(height); ++j)
            split_node(x, y, w, samples);
    }
}

void calculate_histogram_cdf(float* cdf, float* sorted, u32 ct, u32 bins)
{
    memset(histogram,0,bins*sizeof(float));

    float step = (sorted[ct - 1] - sorted[0]) / bins;

    for (int i = 0; i < ct; ++i) {
        float val = sorted[i];
        int bin = int((val - sorted[0]) / step);
        histogram[bin] += 1.f;
    }

    for (int i = 1; i < ct; ++i)
        histogram[i] += histogram[i - 1];

    float m = 1/histogram[ct - 1];
    for (int i = 0; i < ct; ++i)
        histogram[i] *= m;
}

u32 calculate_exact_cdf(float* sorted, u32 ct)
{
    float buffer[ct];
    memcpy(buffer, sorted, ct * sizeof(float));

    for (int i = 1; i < ct; ++i)
        sorted[i] += sorted[i - 1];

    for (int i = 0; i < ct; ++i)
        sorted[i] /= sorted[ct - 1];

    return;
}

float meanerr_branch_sorted(u64* split, float* xti, float* y, float* w, u32 samples, PowerFunc&& powerf)
{
    u64* spl = split;
    u64 flag = 0;
    float mindiff=FLOAT_MAX;
    int breakpoint;

    // create the indices and sort the x values
    pair<uint32_t,float> sort_idxs[samples];

    for (int i = 0; i < samples; ++i)
        sort_idxs[i] = {i,xti[i]};

    sort(sort_idxs,sort_idxs+samples,
    [](pair<uint32_t,float>& a, pair<uint32_t,float>& b)
    { return a.second < b.second; });


    float xt_mean[samples];
    memset(xt_mean,0,samples*sizeof(u32));


    // cumulative sum of all x values (this is to do weights...)
    for (int i = 0; i < samples; ++i)
        xt_mean[i] += xti[sort_idxs[i].first];
 
    for (int i = 1; i < samples; ++i)
        xt_mean[i] += xti[i - 1];

    float sum=xt_mean[samples-1];


    // calculate means for each and see difference
    for (int i = 0; i < samples; ++i) {
        // mean
        float lm=xt_mean[i],rm=xt_mean[samples-1]-lm,diff=0;
        lm /= (i+1);
        rm /= (samples-(i+1));

        // median
        float lm=((i+1)%2==1)?xti[sort_idxs[i>>1].first] : \
                 (0.5*(xti[sort_idxs[i>>1].first]+xti[sort_idxs[(i>>1)].first]));
                 
        float rm=((i+1)%2==1)?xti[sort_idxs[(samples-i)>>1].first] : \
                 (0.5*(xti[sort_idxs[(samples-i)>>1].first]+xti[sort_idxs[((samples-i)>>1)].first]));

        for (int j = 0; j < i; ++j)
            diff += to_include(j) ? 0 : powerf(xti[sort_idxs[i].first]-lm,2);

        for (int j = i; j < samples; ++j)
            diff += to_include(j) ? 0 : powerf(xti[sort_idxs[i].first]-rm,2);

        if (diff<mindiff) {
            breakpoint=i;
            mindiff=diff;
        }
    }

    return xti[breakpoint];
}




/*

    for (int i = 0; i < samples; ++i) {
        mul = y[i] * w[i];

        if (x[i] > lo_bound) {
            avg_l += mul;
            flag  |= (1 << (i & 63));
        } else {
            avg_r += mul;
        }

        if (((i + 1) & 63) == 0) {
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
        diff_sep    = y[i] - (BETWEEN(x[i],lo_bound,hi_bound) ? avg_l : avg_r);

        cost_total += diff_total * diff_total;
        cost_sep   += diff_sep * diff_sep;
    }

*/
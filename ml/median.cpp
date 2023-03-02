
import random

def chunked(l, chunk_size)
{
    """Split list `l` it to chunks of `chunk_size` elements."""
    return [l[i:i + chunk_size] for i in range(0, len(l), chunk_size)]
}

void pick_pivot(l, uint32_t k)
{
    """
    Pick a good pivot within l, a list of numbers
    This algorithm runs in O(n) time.
    """
    assert len(l) > 0

    // If there are < 5 items, just return the median
    if len(l) < 5:
        // In this case, we fall back on the first median function we wrote.
        // Since we only run this on a list of 5 or fewer items, it doesn't
        // depend on the length of the input and can be considered constant
        // time.
        return nlogn_median(l)

    // First, we'll split `l` into groups of 5 items. O(n)
    chunks = chunked(l, 5)

    // For simplicity, we can drop any chunks that arent full. O(n)
    full_chunks = [chunk for chunk in chunks if len(chunk) == 5]


    // Next, we sort each chunk. Each group is a fixed length, so each sort
    // takes constant time. Since we have n/5 chunks, this operation
    // is also O(n)
    sorted_groups = [sorted(chunk) for chunk in full_chunks]

    // The median of each chunk is at index 2
    medians = [chunk[2] for chunk in sorted_groups]

    // It's a bit circular, but I'm about to prove that finding
    // the median of a list can be done in provably O(n).
    // Finding the median of a list of length n/5 is a subproblem of size n/5
    // and this recursive call will be accounted for in our analysis.
    // We pass pick_pivot, our current function, as the pivot builder to
    // quickselect. O(n)
    median_of_medians = quickselect_median(medians, pick_pivot)
    return median_of_medians
}


template <typename T, typename PivotFunc>
void quickselect(T* l, uint32_t k, PivotFunc&& piv)
{
    /*
    Select the kth element in l (0 based)
    :param l: List of numerics
    :param k: Index
    :param pivot_fn: Function to choose a pivot, defaults to random.choice
    :return: The kth element of l
    */
    if len(l) == 1:
        assert k == 0
        return l[0]

    pivot = pivot_fn(l);

    lows = [el for el in l if el < pivot]
    highs = [el for el in l if el > pivot]
    pivots = [el for el in l if el == pivot]

    if (k < len(lows))
        return quickselect(lows, k, pivot_fn);
    else if (k < len(lows) + len(pivots))
        // We got lucky and guessed the median
        return pivots[0];
    else
        return quickselect(highs, k - len(lows) - len(pivots), pivot_fn);
}

template <typename T, typename PivotFunc>
T quickselect_median(T* l, PivotFunc&& piv)
{
    return (len(l) % 2 == 1) ? quickselect(l, len(l) >> 1, pivot_fn) : \
    (0.5 * (quickselect(l, len(l) / 2 - 1, piv) + quickselect(l, len(l) / 2, piv)));
}
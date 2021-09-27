import numpy as np

def gen_1d_buckets(data, num_buckets):
    uq, counts = np.unique(data, return_counts=True)
    if len(uq) <= num_buckets:
        return np.append(uq, uq[-1]+1)
    if len(uq) < 1000000:
        print("Bucketing using manual mode for skewed buckets")
        return gen_buckets_manual(data, num_buckets, uq, counts)
    else:
        print("Bucketing using percentiles for uniform buckets")
        return gen_buckets_percentile(data, num_buckets, uq, counts)

def gen_buckets_percentile(data, num_buckets, uq, counts):
    ptls = np.linspace(0, 100, num_buckets+1)
    breaks = np.percentile(data, ptls, interpolation='nearest')
    assert np.all(np.diff(breaks) > 0), "Data is skewed but used percentile method :("
    breaks[-1] = data.max()+1
    return breaks

def gen_buckets_manual(data, num_buckets, uq, counts):
    num_large_buckets = (counts > (len(data)/num_buckets)).sum()
    count_large_buckets = counts[counts > (len(data)/num_buckets)].sum()
    print("num large buckets = %d, count = %d" % (num_large_buckets, count_large_buckets))
    # Discount the large buckets when setting the target.
    orig_target = (len(data) - count_large_buckets)/(num_buckets - num_large_buckets)
    
    buckets = []
    bucket_counts = []
    cur_count = 0
    total_count = 0
    for u, c in zip(uq, counts):
        target = orig_target
        #if len(buckets) < num_buckets:
        #    new_target = (len(data) - total_count)/(num_buckets - len(buckets)+1)
        #    target = min(target, new_target)
        if len(buckets) == 0:
            buckets.append(u)
            cur_count = 0
        elif cur_count + c > target:
            buckets.append(u)
            bucket_counts.append(cur_count)
            cur_count = 0
        # Everything is going in the current bucket
        cur_count += c
        total_count += c
    bucket_counts.append(cur_count)
    # Buckets also includes the right-most bound
    buckets.append(uq[-1]+1)
    return buckets


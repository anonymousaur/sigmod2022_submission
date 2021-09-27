import numpy as np
import os
import argparse

parser = argparse.ArgumentParser("")
parser.add_argument("--dataset",
        type=str,
        required=True)
parser.add_argument("--dims",
        type=int,
        required=True)
parser.add_argument("--sort-col",
        type=int,
        required=True)
parser.add_argument("--num-buckets",
        type=int,
        required=True)
parser.add_argument("--outfile",
        type=str,
        required=True)
args = parser.parse_args()

data = np.fromfile(args.dataset, dtype=int).reshape(-1, args.dims)
print("Loaded data")

def gen_buckets_percentile(data, num_buckets):
    ptls = np.linspace(0, 100, num_buckets+1)
    breaks = np.percentile(data, ptls, interpolation='nearest')
    assert np.all(np.diff(breaks) > 0), "Data is skewed but used percentile method :("
    breaks[-1] = data.max()+1
    return breaks

col = data[:,args.sort_col]
assert np.all(np.diff(col) >= 0)
print("Validated data")
buckets = gen_buckets_percentile(col, args.num_buckets)
print("Got %d buckets" % (len(buckets)-1))
ix_bounds = np.searchsorted(col, buckets, side='left')

f = open(args.outfile, 'w')
f.write('target_bucket_ranges %d %d\n' % (args.sort_col, len(buckets)-1))
for i in range(1, len(ix_bounds)):
    f.write("%d %d %d %d %d\n" % (i-1, ix_bounds[i-1], ix_bounds[i], buckets[i-1], buckets[i]))
f.close()


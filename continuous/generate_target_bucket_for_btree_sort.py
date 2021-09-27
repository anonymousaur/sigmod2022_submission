import numpy as np
import os
import argparse
from gen_1d_buckets import *

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

col = data[:,args.sort_col]
assert np.all(np.diff(col) >= 0)
print("Validated data")
buckets = gen_1d_buckets(col, args.num_buckets)
print("Got %d buckets" % (len(buckets)-1))
digitized = np.digitize(col, buckets[1:], right=False)
digitized.astype(np.int32).tofile(args.outfile)

bucketfile = os.path.join(os.path.splitext(args.outfile)[0] + "_buckets.dat")
np.savetxt(bucketfile, buckets)


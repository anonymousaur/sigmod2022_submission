from entropy2d import Entropy2D
import numpy as np
import sys
import os

from bucketer import Bucketer, Schema
BASE_DIR="/home/ubuntu/correlations/continuous/"

data = np.fromfile(os.path.join(BASE_DIR, 'stocks/stocks_data_sort_btree4.bin'),
        dtype=int).reshape(-1, 7)

target_ids = np.fromfile(os.path.join(BASE_DIR, 'stocks/target_buckets_btree4.bin'), dtype=np.int32)
b4 = target_ids
print("Got target IDs")

def get_buckets(dim, nbuckets):
    ptls = np.linspace(0, 100, nbuckets+1)
    return np.percentile(data[:,dim], ptls, interpolation='nearest')

s2 = get_buckets(2, 10000)
e2 = Entropy2D(data[:,2], s2, b4)
print("2:", e2.cond_entropy(), e2.weighted_cond_entropy())
s3 = get_buckets(3, 10000)
e3 = Entropy2D(data[:,3], s3, b4)
print("3:", e3.cond_entropy(), e3.weighted_cond_entropy())
s5 = get_buckets(5, 10000)
e5 = Entropy2D(data[:,5], s5, b4)
print("5:", e5.cond_entropy(), e5.weighted_cond_entropy())

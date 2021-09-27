from entropy2d import Entropy2D
import numpy as np
import sys
import os

from bucketer import Bucketer, Schema
BASE_DIR="/home/ubuntu/correlations/continuous/"

data = np.fromfile(os.path.join(BASE_DIR, 'wise/wise_sort_btree9.bin'),
        dtype=int).reshape(-1, 15)

target_ids = np.fromfile(os.path.join(BASE_DIR, 'wise/target_buckets_btree9.bin'), dtype=np.int32)
b9 = target_ids

def get_buckets(dim, nbuckets):
    ptls = np.linspace(0, 100, nbuckets+1)
    return np.percentile(data[:,dim], ptls, interpolation='nearest')

s10 = get_buckets(10, 10000)
e10 = Entropy2D(data[:,10], s10, b9)
print("10:", e10.cond_entropy(), e10.weighted_cond_entropy())
s11 = get_buckets(11, 10000)
e11 = Entropy2D(data[:,11], s11, b9)
print("11:", e11.cond_entropy(), e11.weighted_cond_entropy())
s12 = get_buckets(12, 10000)
e12 = Entropy2D(data[:,12], s12, b9)
print("12:", e12.cond_entropy(), e12.weighted_cond_entropy())
s13 = get_buckets(13, 10000)
e13 = Entropy2D(data[:,13], s13, b9)
print("13:", e13.cond_entropy(), e13.weighted_cond_entropy())
s14 = get_buckets(14, 10000)
e14 = Entropy2D(data[:,14], s14, b9)
print("14:", e14.cond_entropy(), e14.weighted_cond_entropy())



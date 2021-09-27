import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool


def injected_noise_runs():
    fracs = [1, 10, 20, 30, 40, 50]
    
    for f in fracs:
        print("Generating mappings for f =", f)
        target_ids = np.fromfile('synthetic/normal/target_buckets_f%d.bin' % f, dtype=np.int32)
        args = {
            "outdir": "synthetic/normal",
            "name": "gaussian_noise",
            "datafile": \
                "/home/ubuntu/correlations/continuous/synthetic/normal/gaussian_noise_f%d.bin" % f,
            "ncols": 2,
            "target_spec": [Schema(1, bucket_ids=target_ids)],
            "map_spec": Schema(0, num_buckets=10000),
            "k": 1,
            "alphas": [0, 1, 10],
            "suffix": "f%d" % f,
            }
        yield args

def mapped_bucket_runs(): 
    mbs = [10, 100, 300, 1000, 3000, 10000, 30000, 100000, 300000, 1000000] 
    f = 10
    for mb in mbs:
        print("Generating mappings for mb =", mb)
        target_ids = np.fromfile('synthetic/normal/target_buckets_f%d.bin' % f, dtype=np.int32)
        args = {
            "outdir": "synthetic/normal",
            "name": "gaussian_noise",
            "datafile": \
                "/home/ubuntu/correlations/continuous/synthetic/normal/gaussian_noise_f%d.bin" % f,
            "ncols": 2,
            "target_spec": [Schema(1, bucket_ids=target_ids)],
            "map_spec": Schema(0, num_buckets=mb),
            "k": 1,
            "alphas": [1],
            "suffix": "mb%d" % mb,
            }
        yield args

with Pool(processes=5) as pool:
    #for r in pool.imap_unordered(rdset.run, injected_noise_runs()):
    #    print('Done')
    for r in pool.imap_unordered(rdset.run, mapped_bucket_runs()):
        print('Done')



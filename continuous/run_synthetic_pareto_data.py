import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool


def injected_noise_runs():
    dims = [2]
    
    for d in dims:
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

with Pool(processes=5) as pool:
    #for r in pool.imap_unordered(rdset.run, injected_noise_runs()):
    #    print('Done')
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')



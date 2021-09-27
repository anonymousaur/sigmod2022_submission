import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool

SUFFIX = sys.argv[1]

target_ids = np.fromfile('wise/target_buckets_%s.bin' % SUFFIX, dtype=np.int32)
mappings = [(2, 0), (3, 0), (4, 0), (5, 0)]

ALPHAS = [1]

def runs():
    to_run = mappings
    alphas = ALPHAS
    print("Generating mappings for", to_run)
    for mp in to_run:
        args = {
            "outdir": "wise/compressed",
            "name": "wise",
            "datafile": "/home/ubuntu/correlations/continuous/wise/wise_sort_%s.bin" % SUFFIX,
            "ncols": 6,
            "target_spec": [Schema(mp[1], bucket_ids=target_ids)],
            "map_spec": Schema(mp[0], num_buckets=20000),
            "k": 1,
            "alphas": alphas,
            "suffix": "small_" + SUFFIX,
            }
        yield args

with Pool(processes=2) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')


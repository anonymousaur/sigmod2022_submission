import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool

SUFFIX = "f0.20"
DIR = "/home/ubuntu/correlations/continuous/synthetic/target_buckets"

target_ids = np.fromfile(os.path.join(DIR, "target_buckets_%s.bin" % SUFFIX), dtype=np.int32)

def runs():
    for mb in [1000, 3000, 10000, 30000, 100000, 300000, 1000000]:
        args = {
            "outdir": DIR,
            "name": "injected_noise",
            "datafile": os.path.join(DIR, "laplace_injected_noise_%s.bin" % SUFFIX),
            "ncols": 2,
            "target_spec": [Schema(1, bucket_ids=target_ids)],
            "map_spec": Schema(0, num_buckets=mb),
            "k": 1,
            "alphas": [1],
            "suffix": "mb%d_SUFFIX" % mb,
            }
        yield args

with Pool(processes=4) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print("Done")
    #gwl.gen_from_spec(args, "uniform", 1000,
    #        os.path.join(args["name"], "queries_%d_w%d_uniform.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))
    #gwl.gen_from_spec(args, "point", 10000,
    #        os.path.join(args["name"], "queries_%d_w%d_point.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))


import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys

SUFFIX = sys.argv[1]
DIR = "/home/ubuntu/correlations/continuous/synthetic/injected_noise"

target_ids = np.fromfile(os.path.join(DIR, "target_buckets_%s.bin" % SUFFIX), dtype=np.int32)

args1 = {
    "outdir": DIR,
    "name": "injected_noise",
    "datafile": os.path.join(DIR, "laplace_injected_noise_%s.bin" % SUFFIX),
    "ncols": 2,
    "target_spec": [Schema(1, bucket_ids=target_ids)],
    "map_spec": Schema(0, num_buckets=100000),
    "k": 1,
    "alphas": [1],
    "suffix": SUFFIX,
    }

for args in [args1]:
    rdset.run(args)
    #gwl.gen_from_spec(args, "uniform", 1000,
    #        os.path.join(args["name"], "queries_%d_w%d_uniform.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))
    #gwl.gen_from_spec(args, "point", 10000,
    #        os.path.join(args["name"], "queries_%d_w%d_point.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))


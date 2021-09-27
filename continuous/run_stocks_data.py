import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool
import argparse

parser = argparse.ArgumentParser("Stocks Cortex")
parser.add_argument("--suffix",
        type=str,
        required=True,
        help="Suffix to add to each output filename")
parser.add_argument("--outdir",
        type=str,
        default="/home/ubuntu/correlations/continuous/stocks/compressed",
        help="Dir to write output files to")
parser.add_argument("--stats",
        type=str,
        default="",
        help="File containing tuning stats")
args = parser.parse_args()

NAME = "stocks"
SUFFIX = args.suffix
OUTDIR = args.outdir
print("Writing output files to", OUTDIR)

mappings = [(2, 4), (3, 4), (5, 4)]
target_ids = np.fromfile('stocks/target_buckets_%s.bin' % SUFFIX, dtype=np.int32)

def runs():
    for mp in mappings:
        params = {
            "outdir": OUTDIR,
            "name": NAME,
            "datafile": "/home/ubuntu/correlations/continuous/stocks/stocks_data_sort_%s.bin" % SUFFIX,
            "ncols": 7,
            "target_spec": [Schema(mp[1], bucket_ids=target_ids)],
            "map_spec": Schema(mp[0], num_buckets=20000),
            "k": 1,
            "alphas": [-1, 0, 0.2, 0.5, 1, 2, 5],
            "suffix": SUFFIX,
            "stats": args.stats
            }
        yield params

with Pool(processes=3) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')
    #gwl.gen_from_spec(args, "uniform", 1000,
    #        os.path.join(args["name"], "queries_%d_w%d_uniform.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))
    #gwl.gen_from_spec(args, "point", 10000,
    #        os.path.join(args["name"], "queries_%d_w%d_point.dat" % (args["map_spec"].dim,
    #        args["map_spec"].bucket_width))


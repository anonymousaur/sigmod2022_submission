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
        default="/home/ubuntu/correlations/continuous/wise/compressed",
        help="Dir to write output files to")
parser.add_argument("--stats",
        type=str,
        default="",
        help="File containing tuning stats")
args = parser.parse_args()


SUFFIX = args.suffix
OUTDIR = args.outdir

target_ids = np.fromfile('wise/target_buckets_%s.bin' % SUFFIX, dtype=np.int32)
mappings = [(10, 9), (11, 9)] #, (13, 9), (14,9)]
octree_mappings = [(5, 0), (6, 0), (7, 0), (8, 0), (10, 9), (11, 9), (12, 9)]

ALPHAS = [-1, 0, 0.2, 0.5, 1, 2, 5]

def runs():
    to_run = mappings
    alphas = ALPHAS
    if 'octree' in SUFFIX or 'flood' in SUFFIX:
        to_run = octree_mappings
    print("Generating mappings for", to_run)
    for mp in to_run:
        args = {
            "outdir": OUTDIR,
            "name": "wise",
            "datafile": "/home/ubuntu/correlations/continuous/wise/wise_sort_%s.bin" % SUFFIX,
            "ncols": 15,
            "target_spec": [Schema(mp[1], bucket_ids=target_ids)],
            "map_spec": Schema(mp[0], num_buckets=20000),
            "k": 1,
            "alphas": alphas,
            "suffix": SUFFIX,
            }
        yield args

with Pool(processes=2) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')


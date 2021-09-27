import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool
import matplotlib
import argparse

parser = argparse.ArgumentParser("Chicago Taxi Cortex")
parser.add_argument("--suffix",
        type=str,
        required=True,
        help="Suffix to add to each output filename")
parser.add_argument("--outdir",
        type=str,
        default="/home/ubuntu/correlations/continuous/chicago_taxi/compressed",
        help="Dir to write output files to")
parser.add_argument("--stats",
        type=str,
        default="",
        help="File containing tuning stats")
args = parser.parse_args()

NAME = "chicago_taxi"
SUFFIX = args.suffix
OUTDIR = args.outdir

target_ids = np.fromfile('chicago_taxi/target_buckets_%s.bin' % SUFFIX, dtype=np.int32)
mappings = [(4,8), (5,8)]
octree_mappings = [(3, 8), (1,0)]
print("Writing output files to", OUTDIR)

def runs():
    to_run = mappings
    if 'octree' in SUFFIX or 'flood' in SUFFIX:
        to_run.extend(octree_mappings)
    print("Generating mappings for", to_run)
    for mp in to_run:
        args = {
            "outdir": OUTDIR,
            "name": "chicago_taxi",
            "datafile": "/home/ubuntu/correlations/continuous/chicago_taxi/" + \
                    "chicago_taxi_sort_%s.bin" % SUFFIX,
            "ncols": 9,
            "map_spec": Schema(mp[0], num_buckets=20000, mode='flattened'),
            "target_spec": [Schema(mp[1], bucket_ids = target_ids)],
            "k": 1,
            "alphas": [-1, 0,0.2, 0.5, 1, 2, 5],
            "suffix": SUFFIX
        }
        yield args
        

with Pool(processes=3) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')


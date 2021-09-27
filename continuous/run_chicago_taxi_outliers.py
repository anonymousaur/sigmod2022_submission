import run_dataset_with_outliers as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema
import sys
from multiprocessing import Pool
import matplotlib
import argparse

parser = argparse.ArgumentParser("Chicago Taxi Cortex")
parser.add_argument("--outdir",
        type=str,
        default="/home/ubuntu/correlations/continuous/chicago_taxi/compressed/",
        help="Dir to write output files to")
parser.add_argument("--target",
        type=str,
        required=True,
        help="Suffix for the sorted data, i.e. 'flood_0_2_8'")
parser.add_argument("--method",
        type=str,
        required=True,
        help="The method of outlier detection to use")
args = parser.parse_args()

NAME = "chicago_taxi"
OUTDIR = os.path.join(args.outdir, "confusion")
SUFFIX = args.target

mappings = [(4,8), (5,8)]
octree_mappings = [(3, 8), (1,0)]
print("Writing output files to", OUTDIR)

def get_args(target_suffix, outlier_suffix, m):
    target_ids = np.fromfile('chicago_taxi/target_buckets_%s.bin' % target_suffix, dtype=np.int32)
    outfile = ""
    tsuff = ""
    if "flood" in target_suffix:
        tsuff = "flood"
    if "octree" in target_suffix:
        tsuff = "octree"

    if outlier_suffix == "flood":
        outfile = os.path.join(args.outdir,
            'chicago_taxi_a1_flood_0_2_8.k1.%d_%d.outliers' % (m[0], m[1]))
    elif outlier_suffix == "octree":
        outfile = os.path.join(args.outdir,
            'chicago_taxi_a1_octree_0_2_8_p10000.k1.%d_%d.outliers' % (m[0], m[1]))
    elif outlier_suffix == "isoforest":
        outfile = os.path.join('/home/ubuntu/correlations/continuous/outlier_detection',
            'chicago_taxi',
            'outliers_%s_0_2_8_isoforest_%d.outliers' % (tsuff, m[0]))
    elif outlier_suffix == "dbscan":
        outfile = os.path.join('/home/ubuntu/correlations/continuous/outlier_detection',
                'chicago_taxi',
                'outliers_%s_0_2_8_dbscan_%d.outliers' % (tsuff, m[0]))
    elif outlier_suffix == "lof":
        outfile = os.path.join('/home/ubuntu/correlations/continuous/outlier_detection',
            'chicago_taxi',
            'outliers_%s_0_2_8_lof_%d.outliers' % (tsuff, m[0]))
    else:
        print("outlier_suffix %s not recognized" % outlier_suffix)


    return {
            "outdir": OUTDIR,
            "name": "outliers_%s" % outlier_suffix,
            "datafile": "/home/ubuntu/correlations/continuous/chicago_taxi/" + \
                    "chicago_taxi_sort_%s.bin" % SUFFIX,
            "ncols": 9,
            "map_spec": Schema(m[0], num_buckets=20000, mode='flattened'),
            "target_spec": [Schema(m[1], bucket_ids = target_ids)],
            "k": 1,
            "suffix": SUFFIX,
            "outlier_file": outfile
        }

def runs():
    to_run = mappings
    if 'octree' in SUFFIX or 'flood' in SUFFIX:
        to_run.extend(octree_mappings)
    print("Generating mappings for", to_run)
    for mp in to_run:
        ag = get_args(SUFFIX, args.method, mp)
        yield ag
        

with Pool(processes=3) as pool:
    for r in pool.imap_unordered(rdset.run, runs()):
        print('Done')


from stash import Stasher
import numpy as np
import sys
import os
import argparse

# Given a dataset, and the source and target columns, generates the
# naive mapping (no outliers), as well as the filtered mapping (with outliers considered)
# and outlier list.

parser = argparse.ArgumentParser("GenContinuousMapping")
parser.add_argument("--dataset",
        type = str,
        required=True,
        help="Filename with the dataset. If binary, must end with .bin, .npy, or .dat" + \
                ", --dim and --dtype must be set")
parser.add_argument("--dim",
        type = int,
        default=0,
        help="Dimensionality of dataset if --dataset is a binary file")
parser.add_argument("--dtype",
        type = str,
        default = "",
        help = "numpy datatype used to encode the dataset if it's binary")
parser.add_argument("--avg-query-width",
        type = float,
        required=True,
        help = "The width of the typical query on the mapped dimension")
parser.add_argument("--output-rewriter-base",
        type = str,
        required=True,
        help="Filebase for the rewriter. The naive will have a .initial extension, and the " + \
                "filtered one will have a .final extension")
parser.add_argument("--output-outliers",
        type = str,
        required=True,
        help="Filename in which to dump the index of the outliers (as a binary of uint64s)")
parser.add_argument("--mapped-dim",
        type = int,
        required=True,
        help = "Dimension being mapped from")
parser.add_argument("--target-dim",
        type = int,
        required = True,
        help = "Dimension being mapped to")
args = parser.parse_args()

def get_dataset():
    if os.path.splitext(args.dataset)[1] in [".bin", "npy", ".dat"]:
        assert (args.dim and args.dtype)
        return np.fromfile(args.dataset, dtype=args.dtype).reshape(-1, args.dim)
    else:
        return np.loadtxt(args.dataset)

cm_mapping = args.output_rewriter_base + ".initial"
outlier_mapping = args.output_rewriter_base + ".final"

data = get_dataset()
assert (args.avg_query_width > 0)
s = Stasher(data, args.avg_query_width, args.mapped_dim, args.target_dim)
print("Finished initializing Stasher")
open(cm_mapping, 'w').write(s.get_mapping().to_string())
print("Finished writing %s" % cm_mapping)
outlier_ixs, so = s.stash_all_outliers(int(len(data) * 0.25))
print("Finshed finding outliers: %d outliers, overhead reduction = %.2fx" % \
        (len(outlier_ixs), float(so[0][1])/so[-1][1]))
open(outlier_mapping, 'w').write(s.get_mapping().to_string())
print("Finished writing %s" % outlier_mapping)
outlier_ixs.astype(np.uint64).tofile(args.output_outliers)
print("Finished writing %s" % args.output_outliers)



import numpy as np
from sklearn import linear_model as lm
import os
import sys
import argparse

parser = argparse.ArgumentParser("MeasureBeta Fit")
parser.add_argument("--raw-results",
        type=str,
        required=True,
        help="Raw results file output by binary")
parser.add_argument("--output",
        type=str,
        default="",
        help="If set, json file with final stats")

raw_file = sys.argv[1]

query_times = []
ranges = []
lists = []
orig_lists = []

for line in open(raw_file):
    line = line.strip()
    if line.startswith("Unioning"):
        p = line.split()
        orig_lists.append(int(p[5]))
    elif line.startswith("True"):
        p = line.split()
        ranges.append(int(p[5].strip(',')))
        lists.append(int(p[-1]))
        if len(lists) > len(orig_lists):
            orig_lists.append(lists[-1])
    elif line.startswith("Query"):
        p = line.split()
        query_times.append(float(p[-1]))
        assert len(query_times) == len(ranges)
        assert len(query_times) >= len(orig_lists)

query_times = np.array(query_times)
ranges = np.array(ranges)
lists = np.array(lists)
orig_lists = np.array(orig_lists)


features = np.vstack((ranges, lists)).T
features_both = np.vstack((ranges, lists, orig_lists)).T

m = lm.LinearRegression().fit(features, query_times)
print("Coefficients: (ranges)", m.coef_[0], ", (lists)", m.coef_[1], "intercept: ", m.intercept_)
r2 = m.score(features, query_times)
print("Correlation strength (R^2):", r2)

if len(args.output) > 0:
    stats = {
        "beta": m.coef_[1] / m.coef_[0],
        "beta_raw": {
            "range_coef": m.coef_[0],
            "list_coef": m.coef_[1],
            "intercept": m.intercept_,
            "R^2": r2
            }
        }
    outfile = open(args.output)
    json.dump(stats, outfile)


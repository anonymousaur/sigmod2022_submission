import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *
from plot_range_ratio_breakdown import *
import re

MARKERSIZE = 10
DATASET = "chicago_taxi"

host = "octree_0_2_8_p10000"
all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/chicago_taxi/results')
results = match(all_results, lambda r: host in r["name"])
for s in ["0001", "001", "01", "05"]:
    results_s = match(results, \
            lambda r: r["workload"].endswith("queries_1_3_4_5_s%s.dat" % s))
    plot_range_breakdown(DATASET, results_s, "plots/compressed/range_comparison_%s_s%s.png" % (host,
        s))




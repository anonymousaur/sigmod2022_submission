import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *
import plot_range_ratio_breakdown as rrb
import re

MARKERSIZE = 10
DATASET = "stocks"

all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/stocks/old_results')
results_corrix = match(all_results, CORRIX(DATASET))
for s in ["0001", "001", "01", "05"]:
    results_corrix_s01 = match(results_corrix, \
            lambda r: r["workload"].endswith("queries_2_3_4_5_s%s.dat" % s))
    rrb.plot_range_ratio_breakdown(DATASET, results_corrix_s01, "plots/range_ratio_breakdown_s%s.png" % s)




import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *
from plot_list_scan_ratio_by_selectivity import *
import re

MARKERSIZE = 10
DATASET = "wise"

matplotlib.rcParams['figure.figsize'] = [8, 6]
plt.rc('font', size=40)          # controls default text sizes
plt.rc('axes', titlesize=40)     # fontsize of the axes title
plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
plt.rc('legend', fontsize=20)    # legend fontsize

for suff in ["btree9", "octree_0_1_9_p10000"]:
    all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/wise/results')
    results = match(all_results, lambda r: suff in r["name"])
    plot_performance_breakdown(DATASET, results,
            "plots/scanned_ratio_by_sel_%s.png" % suff)




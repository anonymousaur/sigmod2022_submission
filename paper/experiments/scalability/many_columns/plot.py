import os
import sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
sys.path.insert(1, '../../../plot_scripts')
from plot_utils import *


DATASET = "many_buckets"
def plot(results, outbase):
    matplotlib.rcParams['figure.figsize'] = [8,6]
    plt.rc('font', size=36)          # controls default text sizes
    plt.rc('axes', titlesize=28)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize
    secondary = match(results, SECONDARY(DATASET))
    corrix = match(results, CORRIX(DATASET))

    def ncols(r):
        return int(r['name'][-2:])

    def get_latest_per_col(res):
        by_col = defaultdict(list)
        for r in res:
            by_col[ncols(r)].append(r)
        ret = {}
        for c, rs in by_col.items():
            ret[c] = get_latest(rs)[0]
        return ret

    secondary = get_latest_per_col(secondary)
    corrix = get_latest_per_col(corrix)

    col_keys = sorted(corrix.keys())
    for c in col_keys:
        if c not in secondary:
            secondary[c] = {'avg_query_time_ns': 0, 'index_size': 0}

    x = np.arange(len(col_keys))
    plt.clf()
    width=0.3
    plt.bar(x-width/2, [float(corrix[k]['avg_query_time_ns'])/1e6 for k in col_keys],
        width=width, color=COLORS["corrix"], label=BASELINE_NAMES["corrix"])
    plt.bar(x+width/2, [float(secondary[k]['avg_query_time_ns'])/1e6 for k in col_keys],
        width=width, color=COLORS["secondary"], label=BASELINE_NAMES["secondary"])
    plt.xticks(x, [str(c) for c in col_keys])
    plt.xlabel("Number of indexes")
    plt.ylabel("Avg query time (ms)")
    plt.legend()
    plt.savefig(outbase + "_time.png", bbox_inches='tight', pad_inches=0)

    plt.clf()
    plt.bar(x-width/2, [(float(corrix[k]['index_size'])*2.2)/1e9 for k in col_keys],
        width=width, color=COLORS["corrix"], label=BASELINE_NAMES["corrix"])
    plt.bar(x+width/2, [(float(secondary[k]['index_size'])*2.2)/1e9 for k in col_keys],
        width=width, color=COLORS["secondary"], label=BASELINE_NAMES["secondary"])
    plt.xticks(x, [str(c) for c in col_keys])
    plt.xlabel("Number of indexes")
    plt.ylabel("Index size (GB)")
    plt.legend()
    plt.savefig(outbase + "_size.png", bbox_inches='tight', pad_inches=0)


all_results = parse_results('/home/ubuntu/correlations/paper/experiments/scalability/many_columns/results')
plot(all_results, "plots/many_columns")



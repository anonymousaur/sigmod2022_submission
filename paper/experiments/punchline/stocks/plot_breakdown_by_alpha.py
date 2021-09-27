import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper')
from plot_utils import *
from matplotlib.patches import Patch

DATASET = "stocks"

def get_latest_points_scanned(results):
    r = get_latest(results)[0]
    return float(r["avg_scanned_points_in_range"]), \
        float(r["avg_scanned_points_in_list"])

def get_alphas(results, alphas):
    d = {}
    for a in alphas:
        d[a] = match(results, lambda r: f"{DATASET}_a{a}_" in r["name"])
    return d

def plot_breakdown_by_alpha(results, outfile):
    plt.clf()
    res = {} 
    res["octree"] = match(results, OCTREE(DATASET))
    res["secondary"] = match(results, SECONDARY(DATASET))
    res["corrix"] = match(results, CORRIX(DATASET))
    res["hermit"] = match(results, HERMIT(DATASET))
    res["cm"] = match(results, CM(DATASET))
    alphas = [0,1,2,5,10]
    alpha_res = get_alphas(res["corrix"], alphas)
   
    def add_bar(x, name, width):
        scans = get_latest_points_scanned(res[name])
        plt.bar(x, scans[0], color=COLORS[name], width=width)
        plt.bar(x, scans[1], bottom=scans[0], color=COLORS[name], alpha=0.5, width=width)
        print("Added", name)

    def add_alpha_bar(x, a, width):
        scans = get_latest_points_scanned(alpha_res[a])
        plt.bar(x, scans[0], color=COLORS["corrix"], width=width)
        plt.bar(x, scans[1], bottom=scans[0], color=COLORS["corrix"], alpha=0.5, width=width)
        print("Added alpha =", a)

    start_ix = 1
    ticks = []
    if len(res['octree']) > 0:
        add_bar(0, "octree", 0.5)
        ticks.append("Octree")
        start_ix = 0
    add_bar(1, "secondary", 0.5)
    ticks.append("Secondary")
    add_bar(2, "hermit", 0.5)
    ticks.append("Hermit")
    add_bar(3, "cm", 0.5)
    ticks.append("CM")
    for i, a in enumerate(alphas):
        add_alpha_bar(4+i, a, 0.5)
        ticks.append(r"$\alpha = %d$" % a)

    plt.xticks(range(start_ix, 4+len(alphas)), ticks)
    plt.ylabel("Points scanned")
    
    legend = [Patch(facecolor='#555555', edgecolor='#555555', label='Ranges'),
              Patch(facecolor='#555555', edgecolor='#555555', alpha=0.5, label='Point Scans')]
    plt.legend(handles=legend)
    plt.savefig(outfile)


all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/stocks/results')
for suff in ["btree4", "0_4_p1000", "0_4_p10000"]:
    for sel in ["0001", "001", "01", "05"]:
        workload_base = "queries_0_2_3_4_5" if 'btree' not in suff else "queries_2_3_4_5"
        workload = workload_base + "_s%s.dat" % sel
        results = match(all_results, lambda r: 'workload' in r and workload in r["workload"])
        print("Found %d matching results for suffix and sel" % len(results))
        outfile = "plots/points_breakdown_%s_s%s.png" % (suff, sel)
        plot_breakdown_by_alpha(results, outfile)



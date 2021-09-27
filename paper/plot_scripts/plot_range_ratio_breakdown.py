import matplotlib
matplotlib.use('Agg')
from matplotlib.patches import Patch
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
from plot_utils import *
import re

def get_breakpoints(col_heights):
    col_heights.sort()
    return col_heights[1]*1.1, col_heights[2]*0.5

def plot_range_breakdown(dataset, results, outfile):
    matplotlib.rcParams['figure.figsize'] = [15, 6]
    plt.rc('font', size=28)          # controls default text sizes
    plt.rc('axes', titlesize=28)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=20)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=20)    # fontsize of the tick labels
    plt.rc('legend', fontsize=18)    # legend fontsize
    
    def get_alpha(r):
        m = re.search("_a([\.\d]+)_", r["name"])
        if m is None:
            return None
        return m.group(1)

    def get_selectivity(r):
        m = re.search("_s([\d]+)\.dat", r["workload"])
        return m.group(1) 

    sels = [get_selectivity(r) for r in results]
    assert all(x == sels[0] for x in sels), "Not all selectivities are the same!"
    sel = parse_selectivity(sels[0])
    target = sel * NUM_RECORDS[dataset] / 1e6

    def get_point_breakdown(r):
        return float(r["avg_scanned_points_in_range"]), \
                float(r["avg_scanned_points_in_list"])

    corrix = match(results, CORRIX(dataset))
    alphas = [get_alpha(r) for r in corrix]
    alphas_num = [float(a) for a in alphas]
    sort = np.argsort(alphas_num)
    corrix = [corrix[i] for i in sort]
    
    all_results = [
            #("octree", get_latest(match(results, OCTREE(dataset)))),
            ("hermit", get_latest(match(results, HERMIT(dataset)))),
            ("cm", get_latest(match(results, CM(dataset)))),
            ("secondary", get_latest(match(results, SECONDARY(dataset)))),
            ("corrix", corrix)
            ]
    if len(all_results[0][1]) == 0:
        all_results = all_results[1:]

    print("Plotting %d results" % sum(len(r[1]) for r in all_results))
    x = np.arange(len(alphas) + len(all_results)-1)
    plt.clf()
    labels = [BASELINE_NAMES[n[0]] for n in all_results][:-1] + [r"$\alpha = %s$" % alphas[ix] for ix in sort]
    height_ratio = 2.0
    fig, (ax_top, ax_bottom) = plt.subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [1,
        height_ratio]})
    plt.subplots_adjust(hspace=None)
    maxes = []
    for i, (n, res) in enumerate(all_results):
        pts = [get_point_breakdown(r) for r in res]
        ranges = np.array([p[0] for p in pts]) / 1e6
        lists = np.array([p[1] for p in pts]) / 1e6
        ax_top.bar(x[i:i+len(pts)], ranges, color=COLORS[n], alpha=0.6)
        ax_bottom.bar(x[i:i+len(pts)], ranges, color=COLORS[n], alpha=0.6)
        ax_top.bar(x[i:i+len(pts)], lists, bottom=ranges, color=COLORS[n], alpha=0.3, hatch='/')
        ax_bottom.bar(x[i:i+len(pts)], lists, bottom=ranges, color=COLORS[n], alpha=0.3, hatch='/')
        print(n, ": ranges ", ranges, ", lists ", lists)
        maxes.append(max(ranges) + max(lists))
    breakpoints = get_breakpoints(maxes)
    # Use the same scale, so it should occupy the same y range
    ax_top.spines["bottom"].set_visible(False)
    ax_top.set_ylim((breakpoints[1], max(maxes)*1.05))
    ax_bottom.spines["top"].set_visible(False)
    ax_bottom.xaxis.tick_bottom()
    ax_top.get_xaxis().set_visible(False)
    ax_bottom.set_xticks(x)
    ax_bottom.set_xticklabels(labels)
    ax_bottom.set_ylim(0, breakpoints[0])

    d = 0.015
    kwargs = dict(transform=ax_top.transAxes, color='k', clip_on=False)
    ax_top.plot((-d, d), (-d, d), **kwargs)
    ax_top.plot((1-d, 1+d), (-d, d), **kwargs)

    kwargs.update(transform=ax_bottom.transAxes)
    ax_bottom.plot((-d, d), (1-d, 1+d), **kwargs)
    ax_bottom.plot((1-d, 1+d), (1-d, 1+d), **kwargs)

    #ax_top.axhline(target, color="#555555", linewidth=3, linestyle='--')
    #ax_bottom.axhline(target, color="#555555", linewidth=3, linestyle='--')
    ax_bottom.set_xlabel(SYSTEM_NAME)
    ax_bottom.xaxis.set_label_coords(0.7, -0.2)
    ax_top.set_title("Scan Overhead")
    ax_bottom.text(-0.075, 1.6, "Points Scanned (x1M)", rotation=90, transform=ax_bottom.transAxes)
    legend = [ 
              Patch(facecolor='#555555', edgecolor='#555555', alpha=0.6, label='Ranges'),
              Patch(facecolor='#555555', edgecolor='#555555', alpha=0.3, label='Points', hatch='/')
              ]
    ax_top.legend(handles=legend, loc="upper right")
    plt.tight_layout()
    plt.savefig(outfile, pad_inches=0, bbox_inches="tight")


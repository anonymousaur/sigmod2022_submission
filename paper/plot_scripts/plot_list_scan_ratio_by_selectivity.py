import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
from plot_utils import *
import re
from collections import defaultdict
from matplotlib.patches import Patch

def get_selectivity(r):
    m = re.search("_s([\d]+)\.dat", r["workload"])
    return m.group(1) 

def plot_list_fraction(ax, dataset, results):
    def get_point_breakdown(r):
        return float(r["avg_scanned_points_in_list"]), float(r["total_pts"])/1000

    results_by_sel = defaultdict(list)
    for r in results:
        results_by_sel[get_selectivity(r)].append(r)
    results = [get_latest(r)[0] for r in results_by_sel.values()]

    sels = [parse_selectivity(get_selectivity(r)) for r in results]
    pts = [get_point_breakdown(r) for r in results]
    sort = np.argsort(sels)
    print("Plotting %d results" % len(results))
    x = np.arange(len(sels))
    labels = [str(100*sels[ix])+"%" for ix in sort]
    frac = np.array([pts[ix][0] / (pts[ix][1]) for ix in sort])
    ax.bar(x, frac, color=COLORS["corrix"])
    #plt.bar(x, 1-frac, bottom=frac, color=COLORS["corrix"], alpha=0.5, label="Point lookups")
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_xlabel("Selectivity")
    ax.set_ylabel("Fraction of Point Scans")

def plot_normalized_time_spent(ax, dataset, corrix_results, secondary_results):
    corrix_results_by_sel = defaultdict(list)
    sec_results_by_sel = defaultdict(list)
    for r in corrix_results:
        corrix_results_by_sel[get_selectivity(r)].append(r)
    for r in secondary_results:
        sec_results_by_sel[get_selectivity(r)].append(r)
    corrix_res = [get_latest(r)[0] for r in corrix_results_by_sel.values()]
    sec_res = [get_latest(r)[0] for r in sec_results_by_sel.values()]
     
    def time_breakdown(r):
        return float(r["avg_indexing_time_ns"]), float(r["avg_range_scan_time_ns"]), \
                float(r["avg_list_scan_time_ns"])



    sels = [parse_selectivity(get_selectivity(r)) for r in corrix_res]
    sort = np.argsort(sels)
    corrix_times = [time_breakdown(corrix_res[i]) for i in sort] 
    sec_times = [time_breakdown(sec_res[i]) for i in sort] 
    labels = [str(100*sels[ix])+"%" for ix in sort]

    corrix_index_time = np.array([t[0] for t in corrix_times])
    corrix_range_time = np.array([t[1] for t in corrix_times])
    corrix_list_time = np.array([t[2] for t in corrix_times])
    corrix_total_time = corrix_index_time + corrix_range_time + corrix_list_time
    sec_index_time = np.array([t[0] for t in sec_times])
    sec_range_time = np.array([t[1] for t in sec_times])
    sec_list_time = np.array([t[2] for t in sec_times])
    sec_total_time = sec_index_time + sec_range_time + sec_list_time

    x = np.arange(len(sels))
    width=0.3
    ax.bar(x-width/2, corrix_index_time / corrix_total_time, color=COLORS["corrix"], width=width)
    ax.bar(x-width/2, corrix_range_time / corrix_total_time,
            bottom=corrix_index_time / corrix_total_time, color=COLORS["corrix"], alpha=0.6,
            width=width)
    ax.bar(x-width/2, corrix_list_time / corrix_total_time,
            bottom = (corrix_index_time + corrix_range_time) / corrix_total_time,
            color=COLORS["corrix"], alpha=0.3, width=width)
    ax.bar(x+width/2, sec_index_time / sec_total_time, color=COLORS["secondary"], width=width)
    ax.bar(x+width/2, sec_range_time / sec_total_time,
            bottom=sec_index_time / sec_total_time, color=COLORS["secondary"], alpha=0.6,
            width=width)
    ax.bar(x+width/2, sec_list_time / sec_total_time,
            bottom = (sec_index_time + sec_range_time) / sec_total_time,
            color=COLORS["secondary"], alpha=0.3, width=width)

    ax.set_xlabel("Selectivity")
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Time Spent")
    legend = [Patch(facecolor='#555555', edgecolor='#555555', label='Index'),
              Patch(facecolor='#555555', edgecolor='#555555', alpha=0.6, label='Ranges'),
              Patch(facecolor='#555555', edgecolor='#555555', alpha=0.3, label='Points'),
              ]
    return ax.legend(handles=legend, ncol=3, loc="lower center", bbox_to_anchor=(0.5,1.0),
            borderaxespad=0.01, handletextpad=0.5, columnspacing=1,handlelength=1.5,
            borderpad=0.01, frameon=False)


def plot_breakdown_all(dataset, corrix_results, secondary_results, outfile):
    matplotlib.rcParams['figure.figsize'] = [15, 6]
    plt.rc('font', size=32)          # controls default text sizes
    plt.rc('axes', titlesize=32)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=20)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=20)    # fontsize of the tick labels
    plt.rc('legend', fontsize=18)    # legend fontsize

    plt.clf()
    fig, (ax1, ax2) = plt.subplots(1, 2)
    plot_list_fraction(ax1, dataset, corrix_results)
    legend = [Patch(facecolor=COLORS["corrix"], edgecolor='#555555', label=BASELINE_NAMES["corrix"]),
              Patch(facecolor=COLORS["secondary"], edgecolor='#555555',
              label=BASELINE_NAMES["secondary"])]
    lg1 = ax1.legend(handles=legend, ncol=2, loc="lower center", bbox_to_anchor=(0.5,1.0),
            borderaxespad=0.01, handletextpad=0.5, columnspacing=1,handlelength=1.5,
            borderpad=0.01, frameon=False)
    lg2 = plot_normalized_time_spent(ax2, dataset, corrix_results, secondary_results)
    plt.suptitle(NAMES[dataset] + ": Breakdown")
    plt.gca().xaxis.set_major_locator(plt.NullLocator())
    plt.gca().yaxis.set_major_locator(plt.NullLocator())
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    plt.savefig(outfile, bbox_extra_artists = (lg1, lg2,), pad_inches=0)

def plot_performance_breakdown(dataset, results, outfile):
    def get_alpha(r):
        m = re.search("_a([\.\d]+)_", r["name"])
        if m is None:
            return None
        return m.group(1)
    corrix_results = match(results, CORRIX(dataset))
    sec_results = match(results, SECONDARY(dataset))
    alpha_groups = defaultdict(list)
    for r in corrix_results:
        alpha_groups[get_alpha(r)].append(r)

    for a, res in alpha_groups.items():
        new_outfile = os.path.splitext(outfile)[0] + "_a%s.png" % a
        plot_breakdown_all(dataset, res, sec_results, new_outfile)


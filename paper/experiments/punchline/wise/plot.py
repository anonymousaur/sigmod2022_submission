import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *

DATASET = "wise"
MARKERSIZE = 10

#def plot_pareto(results, outfile):
#    plt.clf()
#    full = match(results, lambda r: "_full" in r["name"])
#    cm = get_latest(match(results, lambda r: "_cm" in r["name"]))
#    print(cm)
#    secondary = get_latest(match(results, lambda r: "_secondary" in r["name"]))
#    octree = get_latest(match(results, lambda r: "_octree" in r["name"]))
#    linears = get_latest(match(results, lambda r: "_hermit" in r["name"]))
#    corrix = match(results, lambda r: "stocks_a" in r["name"] and "_linear" not in r["name"])
#
#    full = get_size_speed(full)
#    cm = get_size_speed(cm)
#    secondary = get_size_speed(secondary)
#    octree = get_size_speed(octree)
#    linears = get_size_speed(linears)
#    corrix = get_size_speed(corrix)
#
#    data_size = NUM_PTS * 8 * 7 / 1e9
#    plt.plot(linears[0] + data_size, linears[1], '.-', markersize=MARKERSIZE, color=COLORS["hermit"],
#            label="Hermit")
#    plt.plot(corrix[0] + data_size, corrix[1], '.-', markersize=MARKERSIZE, color=COLORS["corrix"], label=SYSTEM_NAME)
#    plt.plot(cm[0] + data_size, cm[1], '.-', markersize=MARKERSIZE, color=COLORS["cm"], label = "CM")
#    plt.plot(secondary[0] + data_size, secondary[1], '.-', markersize=MARKERSIZE, color=COLORS["secondary"], label = "secondary")
#    if len(octree[0]) > 0:
#        plt.plot(octree[0] + data_size, octree[1], '.-', markersize=MARKERSIZE, color=COLORS["octree"], label="octree")
#    plt.plot(full[0] + data_size, full[1], '.-', markersize=MARKERSIZE, color=COLORS["full"], label="full")
#    plt.xlabel("Table + Index Size (GB)")
#    plt.ylabel("Avg Query Time (ms)")
#    plt.yscale('log')
#    plt.legend()
#
#    plt.savefig(outfile)
#    
#def plot_selectivity(results, outfile):
#    sels = [0.0001, 0.001, 0.01, 0.05]
#    width = 0.1
#    ticks = []
#
#    plt.clf()
#    full = match(results, lambda r: "_full" in r["name"])
#    plt.axhline(get_size_speed(full)[1][-1], linestyle='--', linewidth=5, color='black')
#    for tick, sel in enumerate(["s0001", "s001", "s01", "s05"]):
#        selres = match(results,
#                lambda r: r["workload"].endswith("2_3_4_5_%s.dat" % sel))
#        cm = get_latest(match(selres, lambda r: "_cm" in r["name"]))
#        secondary = get_latest(match(selres, lambda r: "_secondary" in r["name"]))
#        octree = get_latest(match(selres, lambda r: "_octree" in r["name"]))
#        hermit = get_latest(match(selres, lambda r: "_hermit" in r["name"]))
#        corrix = match(selres, lambda r: "stocks_a1_" in r["name"] and "_linear" not in r["name"])
#        plt.bar(tick - width, get_size_speed(cm)[1], color=COLORS["cm"], width=width)
#        plt.bar(tick - 0, get_size_speed(secondary)[1], color=COLORS["secondary"], width=width)
#        if len(octree) > 0:
#            plt.bar(tick + width, get_size_speed(octree)[1], color=COLORS["octree"], width=width)
#            tick += width
#        plt.bar(tick+width, get_size_speed(hermit)[1], color=COLORS["hermit"], width=width)
#        # Get the second from last alpha value
#        plt.bar(tick+2*width, get_size_speed(corrix)[1], color=COLORS["corrix"], width=width)
#        ticks.append(tick)
#
#    plt.xticks(ticks, ["0.01%", "0.1%", "1%", "5%"])
#    plt.xlabel("Query Selectivity")
#    plt.ylabel("Query Performance (ms)")
#    plt.yscale('log')
#    plt.savefig(outfile)

all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/wise/results')

for suffix in ["s001", "s01", "s0001", "s05"]:
    queries_btree = "queries_10_11_12_%s.dat" % suffix
    results_btree = match(all_results, lambda r: "workload" in r and \
            r["workload"].endswith(queries_btree))
    queries_octree = "queries_5_6_7_8_10_11_12_%s.dat" % suffix
    results_octree = match(all_results, lambda r: "workload" in r and \
            r["workload"].endswith(queries_octree))

    full_scan = match(all_results, lambda r: "_full" in r["name"])[:1]
    
    match_p10000 = match(results_octree, lambda r: 'octree_0_1_9_p10000_' in r["name"] or
            r["name"].endswith('p10000'))
    plot_pareto(DATASET, match_p10000 + full_scan, "plots/compressed/%s_pareto_p10000_%s.png" % (DATASET, suffix))
    
    match_btree = match(results_btree, lambda r: 'btree9' in r["name"])
    plot_pareto(DATASET, match_btree+ full_scan, "plots/compressed/%s_pareto_btree9_%s.png" % (DATASET, suffix))

    match_flood = match(results_octree, lambda r: 'flood' in r["name"])
    plot_pareto(DATASET, match_flood+ full_scan, "plots/compressed/%s_pareto_flood_0_1_9_%s.png" % (DATASET, suffix))




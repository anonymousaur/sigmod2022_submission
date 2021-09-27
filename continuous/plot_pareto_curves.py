import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
from collections import defaultdict

RESULTS_FILE = "/home/ubuntu/correlations/cxx/wider_queries_results.csv"
OUTDIR = "plots"
CM_COLOR = "blue"
OUTLIER_COLOR = "green"
LINEAR_COLOR = "purple"
SECONDARY_COLOR = "black"

PLOTS = {
        "airline.12_9.w5": lambda name: "airline" in name and "12_9" in name and \
                name.endswith("_w5"),
        "airline.12_9.w15": lambda name: "airline" in name and "12_9" in name and \
                name.endswith("_w15"),
        "msft_buildings.2_1.w150000": lambda name: "msft_buildings" in name and \
                "2_1" in name and name.endswith("w150000"),
        "msft_buildings.2_1.w500000": lambda name: "msft_buildings" in name and \
                "2_1" in name and name.endswith("w500000"),
        "msft_buildings.2_1.w1000000": lambda name: "msft_buildings" in name and \
                "2_1" in name and name.endswith("w1000000"),
        }

# Returns the size, runtime pair from a file
def point_from_line(line):
    parts = line.split(',')
    if len(parts) < 14:
        print(line)
    return int(parts[11]), float(parts[14])/1e6

def plot(saveto, point_dict):
    plt.clf()
    outlier_pts = sorted(point_dict["outliers"], key= lambda p: p[0])
    linear_pts = sorted(point_dict["linear"], key = lambda p: p[0])
    if "cm" in point_dict:
        plt.plot([point_dict["cm"][0]], [point_dict["cm"][1]],
            '.', color=CM_COLOR, markersize=10, label='CM')
    if "secondary" in point_dict:
        plt.plot([point_dict["secondary"][0]], [point_dict["secondary"][1]],
            '.', color=SECONDARY_COLOR, markersize=10,
            label='Secondary Index')
    if len(outlier_pts) > 0:
            plt.plot([p[0] for p in outlier_pts],
            [p[1] for p in outlier_pts],
            '.-', color = OUTLIER_COLOR, markersize=10,
            label='Outliers')
    if len(linear_pts) > 0:
        plt.plot([p[0] for p in linear_pts],
                [p[1] for p in linear_pts],
                '.-', color = LINEAR_COLOR, markersize=10,
                label='Linear')

    plt.xscale('log')
    plt.xlabel('Size (Bytes)')
    plt.ylabel('Avg query time (ms)')
    plt.legend()
    plt.savefig(saveto)

def plot_group(group_name, lines):
    pts = {"outliers": [], "linear": []}
    for line in lines:
        name = line.split(',')[1]
        suffix = name.split('.')[0].split('_')[-1]
        if suffix == "cm":
            pts["cm"] = point_from_line(line)
        elif suffix == "secondary":
            pts["secondary"] = point_from_line(line)
        elif suffix == "linear":
            pts["linear"].append(point_from_line(line))    
        elif suffix.startswith('a'):
            pts["outliers"].append(point_from_line(line))
        else:
            assert(False), "unknown name %s" % name
    outfile = os.path.join(OUTDIR, group_name + ".png")
    plot(outfile, pts)

def match(line, filt):
    name = line.split(",")[1]
    return filt(name)

def plot_results(resfile):
    lines = open(resfile).readlines()
    groups = defaultdict(list)
    for line in lines:
        for p, filt in PLOTS.items():
            if match(line, filt):
                groups[p].append(line)
    for name, lines in groups.items():
        plot_group(name, lines)
        
plot_results(RESULTS_FILE)


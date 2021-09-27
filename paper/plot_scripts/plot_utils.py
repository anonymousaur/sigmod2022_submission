import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import re
from collections import defaultdict

SYSTEM_NAME = "Cortex"
COLORS =  {
    "full": "black",
    "secondary": "#800000",
    "cm": "#C8A214",
    "octree": "#921fb4",
    "corrix": "#10289F",
    "hermit": "#1DAC2A"
    }
LINESTYLES = {
        "full": "v-",
        "secondary": "D-",
        "cm": "p-",
        "hermit": "s-",
        "octree": "P-",
        "corrix": "o-"
        }
BASELINE_NAMES = {
        "full": "Full Scan",
        "secondary": "B-Tree",
        "cm": "CM",
        "hermit": "Hermit",
        "octree": "Octree",
        "corrix": SYSTEM_NAME
        }

NUM_RECORDS = {
        "chicago_taxi": 194069509,
        "stocks": 165694909,
        "wise": 198142920
        }

SIZES = {
        "chicago_taxi": 13973004648,
        "stocks": 9278914904,
        "wise": 23777150400
        }

NAMES = {
        "chicago_taxi": "Chicago Taxi",
        "stocks": "Stocks",
        "wise": "WISE"
        }

FILTERS = {
    "secondary": lambda s: lambda r: f"{s}_secondary" in r["name"],
    "octree": lambda s: lambda r: f"{s}_octree" in r["name"],
    "hermit": lambda s: lambda r: f"{s}_hermit" in r["name"],
    "full": lambda s: lambda r: f"{s}_full" in r["name"],
    "corrix": lambda s: lambda r: f"{s}_a" in r["name"] and "linear" not in r["name"],
    "cm": lambda s: lambda r: f"{s}_cm" in r["name"]
    }
SECONDARY = FILTERS["secondary"]
OCTREE = FILTERS["octree"]
HERMIT = FILTERS["hermit"]
FULL_SCAN = FILTERS["full"]
CORRIX = FILTERS["corrix"]
CM = FILTERS["cm"]

def split_kv(filename):
    d = {}
    if os.path.isdir(filename):
        return d
    for line in open(filename):
        p = line.split(':')
        d[p[0].strip()] = p[1].strip()
    return d

def parse_results(results_dir):
    res = []
    for f in os.listdir(results_dir):
        r = split_kv(os.path.join(results_dir, f))
        if 'workload' in r and 'avg_query_time_ns' in r:
            res.append(r)
    return res

def match(results, filt, clean=True):
    matches = []
    for r in results:
        if clean and ('name' not in r or 'workload' not in r or 'avg_query_time_ns' not in r):
            continue
        if filt(r):
            matches.append(r)
    return matches

def get_size_speed(results, sort=True):
    sizes = []
    speeds = []
    for r in results:
        sizes.append(int(r["index_size"]) / 1e9) # Report in GB
        speeds.append(float(r["avg_query_time_ns"]) / 1e6) # Report in ms
    sizes = np.array(sizes)
    speeds = np.array(speeds)
    if sort:
        s = np.argsort(sizes)
        return sizes[s], speeds[s]
    else:
        return sizes, speeds

def get_latest(results):
    if len(results) == 0:
        return []
    srt = sorted(results, key=lambda r: r["timestamp"])
    return [srt[-1]]

def get_alpha(r):
    m = re.search("_a([\.\d]+)_", r["name"])
    if m is None:
        return None
    return m.group(1)

def get_latest_per_alpha(results):
    alphas = defaultdict(list)
    for r in results:
        alphas[get_alpha(r)].append(r)
    final_res = []
    for a, res in alphas.items():
        final_res.extend(get_latest(res))
    return final_res

def get_time_breakdown(results):
    index_times = []
    range_times = []
    list_times = []
    for r in results:
        index_times.append(float(r["avg_indexing_time_ns"]) / 1e6)
        range_times.append(float(r["avg_range_scan_time_ns"]) / 1e6)
        list_times.append(float(r["avg_list_scan_time_ns"])/1e6)

    return index_times, range_times, list_times

def get_scanned_pts(results):
    ranges = []
    points = []
    for r in results:
        ranges.append(float(r["avg_scanned_points_in_range"]))
        points.append(float(r["avg_scanned_points_in_list"]))
    return ranges, points

def parse_selectivity(sel_str):
    if sel_str.startswith('s'):
        sel_str = sel_str[1:]
    return float("." + sel_str)

# Plots the size-speed pareto curves
def plot_pareto(dataset, results, outfile):
    matplotlib.rcParams['figure.figsize'] = [10, 7]
    plt.rc('font', size=36)          # controls default text sizes
    plt.rc('axes', titlesize=36)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize

    plt.clf()
    full = get_latest(match(results, FULL_SCAN(dataset)))
    cm = get_latest(match(results, CM(dataset)))
    secondary = get_latest(match(results, SECONDARY(dataset)))
    octree = get_latest(match(results, OCTREE(dataset)))
    hermit = get_latest(match(results, HERMIT(dataset)))
    corrix = get_latest_per_alpha(match(results, CORRIX(dataset)))
    # Remove alpha = 0
    #corrix = match(corrix, lambda r: '_a0_' not in r["name"] and '_a10_' not in r["name"])
    corrix = match(corrix, lambda r: '_a1_' in r["name"])

    data_size = 0 #SIZES[dataset] / 1e9
    data = []
    data.append(("full", get_size_speed(full)))
    data.append(("corrix", get_size_speed(corrix)))
    data.append(("secondary", get_size_speed(secondary)))
    data.append(("cm", get_size_speed(cm)))
    #data.append(("octree", get_size_speed(octree)))
    data.append(("hermit", get_size_speed(hermit)))

    print(get_size_speed(corrix)[0])
    print(get_size_speed(secondary)[0])

    for n, d in data:
        markersize=30 # if n != "corrix" else 10
        print("%s: %d" % (n, len(d[0])))
        plt.plot(d[0] + data_size, d[1], LINESTYLES[n], linewidth=5, markersize=markersize, color=COLORS[n],
                label=BASELINE_NAMES[n])
    plt.xlabel("Index Size (GB)")
    plt.ylabel("Avg Query Time (ms)")
    plt.yscale('log')

    title = NAMES[dataset]
    btree = False
    if 'btree' in outfile or 'primary' in outfile:
        title += " (1-D Host)"
    elif 'flood' in outfile:
        title += " (Flood Host)"
    else:
        title += " (Octree Host)"

    plt.title(title)
    #plt.margins(0,0)
    #plt.legend()
    plt.tight_layout()
    handles, labels = plt.gca().get_legend_handles_labels()
    plt.savefig(outfile, bbox_inches="tight", pad_inches=0)
    
    #matplotlib.rcParams['figure.figsize'] = [10, 2]
    plt.clf()
    plt.margins(0,0)
    plt.axis("off")
    lgd = plt.legend(handles, labels, ncol = len(handles), loc="lower left", bbox_to_anchor=(0,1))
    legend_file = os.path.splitext(outfile)[0] + "_legend.png"
    print("Printing legend to:", legend_file)
    plt.gca().xaxis.set_major_locator(plt.NullLocator())
    plt.gca().yaxis.set_major_locator(plt.NullLocator())
    #plt.tight_layout()
    plt.savefig(legend_file, bbox_extra_artists=(lgd,), bbox_inches='tight', pad_inches=0)

# Plots the size-speed pareto curves
def plot_vary_alpha(dataset, results, outfile):
    matplotlib.rcParams['figure.figsize'] = [10, 7]
    plt.rc('font', size=36)          # controls default text sizes
    plt.rc('axes', titlesize=36)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize

    plt.clf()
    #full = get_latest(match(results, FULL_SCAN(dataset)))
    #cm = get_latest(match(results, CM(dataset)))
    #secondary = get_latest(match(results, SECONDARY(dataset)))
    #octree = get_latest(match(results, OCTREE(dataset)))
    #hermit = get_latest(match(results, HERMIT(dataset)))
    corrix = get_latest_per_alpha(match(results, CORRIX(dataset)))
    # Remove alpha = 0
    corrix = match(corrix, lambda r: '_a0_' not in r["name"] and '_a10_' not in r["name"])
    #corrix = match(corrix, lambda r: '_a1_' in r["name"])

    data_size = 0 #SIZES[dataset] / 1e9
    data = ("corrix", get_size_speed(corrix), sorted([float(get_alpha(c)) for c in corrix]))

    print(get_size_speed(corrix)[0])

    markersize=30 # if n != "corrix" else 10
    d = data[1]
    plt.plot(d[0] + data_size, d[1], LINESTYLES[data[0]], linewidth=5, markersize=markersize,
            color=COLORS[data[0]],
                label=BASELINE_NAMES[data[0]])
    plt.xlabel("Index Size (GB)")
    plt.ylabel("Avg Query Time (ms)")
    plt.yscale('log')

    for i, t in enumerate(reversed(data[2])):
        plt.gca().annotate(r"$\alpha = %s$" % str(t), (data[1][0][i] + data_size, data[1][1][i]),
                xytext=(15, 10), textcoords='offset points', fontsize=20)

    title = NAMES[dataset]
    btree = False
    if 'btree' in outfile or 'primary' in outfile:
        title += " (1-D Host)"
    elif 'flood' in outfile:
        title += " (Flood Host)"
    else:
        title += " (Octree Host)"

    plt.xlim(right=max(data[1][0])+1)
    plt.ylim(top=max(data[1][1])*1.1)
    plt.title(title)
    #plt.margins(0,0)
    #plt.legend()
    plt.tight_layout()
    handles, labels = plt.gca().get_legend_handles_labels()
    plt.savefig(outfile, bbox_inches="tight", pad_inches=0)
    
    #matplotlib.rcParams['figure.figsize'] = [10, 2]
    plt.clf()
    plt.margins(0,0)
    plt.axis("off")
    
    lgd = plt.legend(handles, labels, ncol = len(handles), loc="lower left", bbox_to_anchor=(0,1))
    legend_file = os.path.splitext(outfile)[0] + "_legend.png"
    print("Printing legend to:", legend_file)
    plt.gca().xaxis.set_major_locator(plt.NullLocator())
    plt.gca().yaxis.set_major_locator(plt.NullLocator())
    #plt.tight_layout()
    plt.savefig(legend_file, bbox_extra_artists=(lgd,), bbox_inches='tight', pad_inches=0)


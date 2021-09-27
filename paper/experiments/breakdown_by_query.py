import numpy as np
import os
import sys
from collections import defaultdict

if len(sys.argv) < 2:
    print("Requires a raw result file")
    sys.exit(1)

raw_result_file = sys.argv[1]

def parse_workload_name(line):
    prefix = "Starting workload"
    p = line.split("(")[0]
    p = p[len(prefix):].strip()
    return p

def get_results_for_workload(rawfile):
    stats = []
    results = {}
    cur_workload = ""
    for line in open(rawfile):
        if 'Starting workload' in line.strip():
            if len(stats) > 0:
                results[cur_workload] = stats
            stats = []
            cur_workload = parse_workload_name(line)
            print("Fetching results for", cur_workload)
        elif line.startswith('Query'):
            p = line.split()
            ix = int(p[1])
            t = float(p[-1])
            stats.append({"time": t})
            assert len(stats) == ix+1, \
                    "Parsed %d queries but only found %d results" % (ix+1, len(stats))
    results[cur_workload] = stats
    return results

def workload_dimensions(workload_file):
    dims = []
    dimix = 0
    for line in open(workload_file):
        if line.startswith("===="):
            if len(dims) > 0:
                yield dims
            dims = []
            dimix = 0
        elif 'none' not in line:
            dims.append(dimix)
            dimix += 1
        else:
            dimix += 1

if __name__ == "__main__":
    for wf, stats in get_results_for_workload(raw_result_file).items():
        avg_time_by_dim = defaultdict(lambda: ([], []))
        for ix, (dims, stat) in enumerate(zip(workload_dimensions(wf), stats)):
            assert len(dims) == 1, "Got dims %s" % str(dims)
            d = dims[0]
            cur = avg_time_by_dim[d]
            cur[0].append(stat["time"])
            cur[1].append(ix)
    
        print("Workload:", wf)
        for dim, avg in avg_time_by_dim.items():
            print("%d, mean %f, median %f, max %f (index %d)" % (dim, np.mean(avg[0]), np.median(avg[0]),
                np.max(avg[0]), avg[1][np.argmax(avg[0])]))


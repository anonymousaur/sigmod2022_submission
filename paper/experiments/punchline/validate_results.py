import os
import sys

RESULTS_DIR=sys.argv[1]

def split_kv(filename):
    d = {}
    for line in open(filename):
        p = line.split(':')
        d[p[0].strip()] = p[1].strip()
    return d

def parse_results(results_dir):
    res = []
    for f in os.listdir(results_dir):
        res.append(split_kv(os.path.join(results_dir, f)))
    return res

def match(results, filt):
    matches = []
    for r in results:
        if filt(r):
            matches.append(r)
    return matches

all_results = parse_results(RESULTS_DIR)
for suffix in ["s001", "s01", "s0001", "s05"]:
    results = match(all_results, lambda r: r["name"].endswith(suffix))

    full_scan = match(results, lambda r: "full" in r["name"])
    assert len(full_scan) == 1
    true_pts = full_scan[0]["total_pts"]
    
    for r in results:
        if r["total_pts"] != true_pts:
            print("Run %s has wrong number of points: got %s but want %s" % (r["name"],
                r["total_pts"], true_pts)) 


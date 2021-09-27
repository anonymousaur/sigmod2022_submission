import numpy as np
import matplotlib
import sys
import os
import json

from bucketer import Schema
from fast_stasher import Stasher
from multiprocessing import Pool

def run_alpha(argv):
    args, stasher, a = argv
    name = args["name"]
    datafile = args["datafile"]
    ncols = args["ncols"]
    map_spec = args["map_spec"]
    target_specs = args["target_spec"]
    k = args["k"]
    alphas = args["alphas"]
    suffix = args["suffix"]
    outdir = args["outdir"] if "outdir" in args else name

    run_cm = a < 0
    if run_cm:
        a = 1
    outlier_bnd = 0 if run_cm else None
    print("Stashing for alpha =", a)
    outlier_ixs, stats = stasher.stash_outliers_fast(a, max_outliers=outlier_bnd)
    uix = np.unique(outlier_ixs, return_counts=True)
    init_so = stats["initial_scan_overhead"]
    final_so = stats["final_scan_overhead"]
    cm_prefix = "cm" if run_cm else "a%s" % str(a)
    full_suffix = "_" + suffix if len(suffix) > 0 else ""
    target_str = '_'.join(str(td.dim) for td in target_specs)
    prefix = "%s/%s_%s%s.k%d.%d_%s" % (outdir, name, cm_prefix, full_suffix, k, map_spec.dim,
            target_str)
    print("Writing mapping for alpha =", a)
    stasher.write_mapping(prefix)
    
    w = open(prefix + ".stats", 'w')
    for key, val in stats.items():
        w.write(key + ":" + str(val) + "\n")
    w.close()
    print("\n=========== alpha = %s (%d => %s) ===========" % ("CM" if run_cm else str(a),
        map_spec.dim, target_str))
    print("Suffix %s" % suffix)
    print("Got %d outliers" % len(outlier_ixs))
    print("Overhead: %d => %d (%.02f%% reduction)" % (init_so, final_so, 100*(init_so - final_so)/init_so))
    return prefix

def run(args):
    print(args)
    name = args["name"]
    datafile = args["datafile"]
    ncols = args["ncols"]
    map_spec = args["map_spec"]
    target_specs = args["target_spec"]
    k = args["k"]
    alphas = args["alphas"]
    suffix = args["suffix"]
    outdir = args["outdir"] if "outdir" in args else name
    
    data = np.fromfile(datafile, dtype=int).reshape(-1, ncols)
    print("Loaded %d data points" % len(data))
    
    non_cm_present = np.any(np.array(alphas) >= 0)
    beta = 17.88
    if 'stats' in args and len(args['stats']) > 0:
        stats = json.load(open(args['stats'])) 
        if 'beta' in stats:
            beta = stats['beta']

    s = Stasher(data, map_spec, target_specs, beta=beta)
    print("Finished initializing Stasher (beta = %f), finding outliers" % beta)

    # process alphas in parallel
    def blocks(alphas):
        for a in alphas:
            yield (args, s, a)

    for a in alphas:
        run_alpha((args, s, a))

    print("\n==================================\n")


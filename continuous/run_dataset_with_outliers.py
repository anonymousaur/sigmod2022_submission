import numpy as np
import matplotlib
import sys
import os
import json

from bucketer import Schema
from fast_stasher import Stasher
from multiprocessing import Pool

def run(args):
    print(args)
    name = args["name"]
    datafile = args["datafile"]
    ncols = args["ncols"]
    map_spec = args["map_spec"]
    target_specs = args["target_spec"]
    k = args["k"]
    assert "alphas" not in args
    suffix = args["suffix"]
    outdir = args["outdir"] if "outdir" in args else name
    
    data = np.fromfile(datafile, dtype=int).reshape(-1, ncols)
    print("Loaded %d data points" % len(data))
    
    stasher = Stasher(data, map_spec, target_specs, beta=1)
    
    outliers = np.fromfile(args["outlier_file"], dtype=int)

    print("Determining target mapping with %d outliers" % len(outliers))

    stasher.stash_given_outliers(outliers)
    cm_prefix = "confusion"
    full_suffix = "_" + suffix if len(suffix) > 0 else ""
    target_str = '_'.join(str(td.dim) for td in target_specs)
    prefix = "%s/%s_%s%s.k%d.%d_%s" % (outdir, name, cm_prefix, full_suffix, k, map_spec.dim,
            target_str)
    stasher.write_mapping(prefix)
    
    print("\n==================================\n")


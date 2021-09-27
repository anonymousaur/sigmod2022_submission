import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *

MARKERSIZE = 10
NUM_PTS = 165694909
DATASET = "stocks"

all_results = parse_results('/home/ubuntu/correlations/paper/experiments/punchline/stocks/results')

for suffix in ["s001", "s01", "s0001", "s05"]:
    queries = "queries_2_3_5_%s.dat" % suffix
    results = match(all_results, lambda r: "workload" in r and r["workload"].endswith(queries))

    full_scan = match(all_results, lambda r: "_full" in r["name"])[:1]
    
    match_p10000 = match(results, lambda r: 'p10000_' in r["name"] or r["name"].endswith('p10000'))
    plot_pareto(DATASET, match_p10000 + full_scan, "plots/compressed/%s_pareto_p10000_%s.png" % (DATASET, suffix))
    
    match_btree = match(results, lambda r: 'btree4' in r["name"])
    plot_pareto(DATASET, match_btree+ full_scan, "plots/compressed/%s_pareto_btree4_%s.png" % (DATASET, suffix))
    
    match_flood = match(results, lambda r: 'flood_0_4' in r["name"])
    plot_pareto(DATASET, match_flood+ full_scan, "plots/compressed/%s_pareto_flood_0_4_%s.png" % (DATASET, suffix))



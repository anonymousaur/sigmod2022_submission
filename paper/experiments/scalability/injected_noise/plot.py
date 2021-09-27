import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_injected_noise import *
from plot_utils import *

DATASET = "injected_noise"
all_results = parse_results('/home/ubuntu/correlations/paper/experiments/scalability/injected_noise/results')

# Plot pareto curves
for suffix in ["s01"]:
    results = match(all_results, lambda r: "workload" in r and r["workload"].endswith(\
            "queries_0_%s.dat" % suffix ))
    plot_noise_benchmark(DATASET, results, "plots/injected_noise")


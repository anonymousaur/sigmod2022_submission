import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys
import os
sys.path.insert(1, '/home/ubuntu/correlations/paper')
from plot_utils import *

DATASET = "gaussian_noise"

def plot_noise_injection(results, outfile):
    fracs = [0, 10, 20, 30, 40, 50]
    def get_data_for(results):
        vals = []
        for f in fracs:
            r = match(results, lambda r: 'f%d' % f in r['name'])
            assert len(r) == 1
            vals.append(float(r[0]['avg_query_time_ns']) / 1e6) 
        return vals

    corrix = get_data_for(match(results, CORRIX(DATASET)))
    secondary = get_data_for(match(results, SECONDARY(DATASET)))
    assert len(corrix) == len(fracs), "Missing some results for Cortex"
    assert len(secondary) == len(fracs), "Missing some results for secondary"

    plt.clf()
    plt.xticks(fracs, ["%d%%" % f for f in fracs])
    plt.plot(fracs, corrix, '-.', markersize=5, color=COLORS["corrix"], label=SYSTEM_NAME)
    plt.plot(fracs, secondary, '-.', markersize=5, color=COLORS["secondary"], label="B+ Tree")
    #plt.axhline(full[1][0], linestyle='--', linewidth=3, color=COLORS["full"])

    plt.xlabel("Injected noise fraction")
    plt.ylabel("Avg Query Time (ms)")
    plt.ylim(ymin=0)
    plt.legend()
    plt.savefig(outfile)


results = parse_results('results')
results_s01 = match(results, lambda r: 's01' in r['workload'])
print ([r['name'] for r in results_s01])
plot_noise_injection(results_s01, 'plots/noise_injection_s01.png')
results_s001 = match(results, lambda r: 's001' in r['workload'])
print ([r['name'] for r in results_s001])
plot_noise_injection(results_s01, 'plots/noise_injection_s001.png')



import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import os
sys.path.insert(1, '/home/ubuntu/correlations/paper')
from plot_utils import *

def plot_curse_of_dim(results, outfile):
    dims = [2,4,6,8,10]
    vals = []
    for d in dims:
        r = match(results, lambda r: 'd%d' % d in r['name'])
        assert len(r) == 1
        vals.append(float(r[0]['avg_scanned_points_in_range'])) 
    plt.clf()
    plt.bar(dims, vals)
    plt.gca().axes.get_xaxis().set_ticks(dims)
    plt.xlabel('# of columns indexed')
    plt.ylabel('Points scanned')
    plt.axhline(100000000, linestyle='--', linewidth=5, color='grey')
    plt.savefig(outfile)


results = parse_results('results')
results_s01 = match(results, lambda r: 's01' in r['workload'])
print ([r['name'] for r in results_s01])
plot_curse_of_dim(results_s01, 'curse_of_dim_s01.png')


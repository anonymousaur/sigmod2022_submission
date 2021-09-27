import numpy as np
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


colors = ["red", "black", "#228B22", "purple", "blue"]
infiles = sys.argv[1:]

def get_times(infile):
    times = []
    points = []
    for line in open(infile):
        if line.startswith("Query time"):
            parts = line.split()[-1]
            times.append(float(parts))
        if line.startswith("True:"):
            pts = int(line[5:].split(',')[0])
            points.append(pts)
    print(infile + ":")
    ptl_levels = [5, 10, 25, 50, 75, 90, 95, 99, 99.9, 100]
    time_ptls = np.percentile(times, ptl_levels)
    points_ptls = np.percentile(points, ptl_levels)
    for (tp, pp, l) in zip(time_ptls, points_ptls, ptl_levels):
        print("%.1fth ptl (%d pts): %.2f ms" % (l, int(pp), tp))
    print("Average:", np.mean(times))
    return np.array(times)

def add_cdf(times, color):
    vals = np.sort(times)
    ys = np.linspace(0, 100, len(vals))
    plt.plot(vals, ys, '-', c=color)

plt.xscale('log')
plt.axvline(689.645)
for i, f in enumerate(infiles):
    add_cdf(get_times(f), colors[i])
    plt.savefig('result_plots/cdf.png')


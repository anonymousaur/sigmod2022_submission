import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import random
from datetime import datetime, timedelta
from pytz import utc
import sys
import itertools
import os

matplotlib.rcParams['figure.figsize'] = [6,5]
plt.rc('font', size=28)          # controls default text sizes
plt.rc('axes', titlesize=30)     # fontsize of the axes title
plt.rc('axes', labelsize=28)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=32)    # fontsize of the tick labels
plt.rc('ytick', labelsize=32)    # fontsize of the tick labels
plt.rc('legend', fontsize=34)    # legend fontsize

suffix = sys.argv[1]
BASE_DIR = "/home/ubuntu/correlations/continuous/stocks"

data = np.fromfile(os.path.join(BASE_DIR, "stocks_data_sort_%s.bin" % suffix),
        dtype=int).reshape(-1, 7)
num_pts = len(data)
ixs = random.sample(range(len(data)), 100000)
print("Done sampling")

name = {
        2: "High Price", 
        3: "Low Price",
        4: "Open Price",
        5: "Close Price",
        }

def load_outliers(dim0, dim1):
    for a in ["a0"]: # , "a1"]:
        filename = "stocks_%s_%s.k1.%d_%d.outliers" % (a, suffix, dim0, dim1)
        filename = os.path.join(BASE_DIR, filename)
        if not os.path.exists(filename):
            return None
        yield a, np.fromfile(filename, dtype=int)



def split_sample(data, ixs, outliers):
    sample_outliers = np.intersect1d(ixs, outliers)
    sample_inliers = np.setdiff1d(ixs, outliers)
    return data[sample_inliers,:], data[sample_outliers,:]

def nonulls(data1, data2):
    return data1, data2
    


def plot_outliers(pair, outliers, alpha_str):
    print("%d => %d: %f%% outliers" % (pair[0], pair[1], 100*len(outliers)/num_pts))
    if outliers is None:
        print("Couldn't load outliers for %d => %d" % pair)
        sys.exit(0)
    inlier_data, outlier_data = split_sample(data, ixs, outliers) 
    inliers_d1, inliers_d2 = inlier_data[:, pair[0]], inlier_data[:, pair[1]]
    inliers_d1, inliers_d2 = nonulls(inliers_d1, inliers_d2)
    outliers_d1, outliers_d2 = outlier_data[:, pair[0]], outlier_data[:, pair[1]]
    outliers_d1, outliers_d2 = nonulls(outliers_d1, outliers_d2) 

    ax = plt.gca()
    ax.scatter(outliers_d1, outliers_d2, s=1, c='red')
    ax.scatter(inliers_d1, inliers_d2, s=1, c='blue')
    #if pair[0] not in [0, 1, 5, 6, 7, 8]:
    #    ax.set_xlim(xmin=0)
    #    ax.set_ylim(ymin=0)
    ax.set_xticklabels([])
    ax.set_yticklabels([])
    ax.set_xlabel(name[pair[0]])
    ax.set_ylabel(name[pair[1]])
    plt.title(r"$\alpha = $ %s" % alpha_str[1:])
    ax.set_xlim(4000, 5000)
    ax.set_ylim(4000, 5000)
    output_dir = '/home/ubuntu/correlations/paper/experiments/punchline/stocks'
    f = os.path.join(output_dir, 'plots/outliers_%s_%s_%d_%d.png' % \
            (suffix, alpha_str, pair[0], pair[1]))
    plt.savefig(f)
    print("Wrote", f)

plt.clf()
#pairs = [(10, 9), (11, 9), (12, 9), (13, 9), (14, 9)]
pairs = [(5, 4), (2, 4)]
for pair in pairs:
    plt.clf()
    outliers = load_outliers(pair[0], pair[1])
    for a, out in load_outliers(pair[0], pair[1]):
        plot_outliers(pair, out, a) 


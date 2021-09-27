import numpy as np
from sklearn.neighbors import LocalOutlierFactor
from sklearn.ensemble import IsolationForest
from sklearn.cluster import DBSCAN
import argparse
import random

METHODS = ["lof", "isoforest", "dbscan", "dbscan_test"]

parser = argparse.ArgumentParser("Gen outliers")
parser.add_argument("--dataset",
        type=str,
        required=True)
parser.add_argument("--dim",
        type=int,
        required=True)
parser.add_argument("--cols",
        type=int,
        nargs="+",
        required=True)
parser.add_argument("--method",
        type=str,
        choices=METHODS,
        required=True)
parser.add_argument("--output",
        type=str,
        required=True)
args = parser.parse_args()

assert len(args.cols) > 1

data = np.fromfile(args.dataset, dtype=int).reshape(-1, args.dim)
data = data[:, args.cols]
perm = np.random.permutation(len(data))
inv = np.argsort(perm)

print("Loaded data")
if args.method == "lof":
    clf = LocalOutlierFactor(n_neighbors=100)
    print("Created LOF outlier detector")
    outliers = []
    div = 100
    for i in range(div):
        start = int(len(data) * i/div)
        end = int(len(data) * (i+1)/div)
        p = perm[start:end]
        res = clf.fit_predict(data[p])
        print("Fit chunk %d of data" % i)
        outliers.extend(list(inv[p[np.where(res < 0)[0]]]))
    outliers = np.where(res < 0)
elif args.method == "isoforest":
    div = 100
    print("Created Isolation Forest detector")
    outliers = []
    print("finished fitting")
    for i in range(div):
        start = int(len(data) * i/div)
        end = int(len(data) * (i+1)/div)
        p = perm[start:end]
        clf = IsolationForest(max_samples=0.1,
                n_jobs=-1, n_estimators=10)
        res = clf.fit(data[p]).predict(data[p])
        print("Fit chunk %d of data" % i)
        outliers.extend(list(inv[p[np.where(res < 0)[0]]]))
elif args.method == "dbscan_test":
    div = 100
    mins = np.min(data, axis=0)
    maxs = np.max(data, axis=0)
    md = np.sqrt(np.sum((maxs-mins)**2))
    print("Max distance = %f" % md)
    eps_candidates = np.linspace(2, 1001, div)
    p = perm[0:int(len(data)/100)]
    outlier_frac = 1.0
    min_eps = 1000
    max_eps = 50000
    i = 0
    while True:
        eps = (min_eps + max_eps)/2
        print("Iteration %i, eps = %d" % (i, eps))
        clf = DBSCAN(eps=eps, min_samples=100, leaf_size=1000)
        res = clf.fit(data[p, :])
        outliers = np.where(res.labels_ < 0)[0]
        outlier_frac = len(outliers) * div/len(data)
        print("eps = %f: %f" % (eps, len(outliers) * div/len(data)))
        if outlier_frac < 0.09:
            max_eps = eps
        elif outlier_frac > 0.11:
            min_eps = eps
        else:
            break
        del clf
        del outliers
elif args.method == "dbscan":
    div = 100
    mins = np.min(data, axis=0)
    maxs = np.max(data, axis=0)
    EPS = 7125
    if 1 in args.cols:
        EPS = 9804
    clf = DBSCAN(eps=EPS, min_samples=100, leaf_size=1000)
    outliers = []
    for i in range(div):
        start = int(len(data) * i/div)
        end = int(len(data) * (i+1)/div)
        p = perm[start:end]
        res = clf.fit(data[p])
        outlier_ixs = np.where(res.labels_ < 0)[0]
        print("Fit chunk %d of data" % i)
        outliers.extend(list(inv[p[outlier_ixs]]))

if args.output:
    np.array(outliers).tofile(args.output)



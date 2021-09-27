import numpy as np
import sys


infile = sys.argv[1]
match_times = []
scan_times = []
bucket_lists = []
for line in open(infile):
    if line.startswith("Match time"):
        parts = line.split()[-1]
        match_times.append(float(parts))
    if line.startswith("Scan time"):
        parts = line.split()[-1]
        scan_times.append(float(parts))
    if line.startswith("Unioning"):
        parts = line.split()[1]
        bucket_lists.append(int(parts))
match_times = np.array(match_times)
scan_times = np.array(scan_times)
bucket_lists = np.array(bucket_lists).astype(int)
ptl_levels = [5, 10, 25, 50, 75, 90, 100]
if len(match_times) == 0:
    match_times = np.array([0])
if len(bucket_lists) == 0:
    bucket_lists = np.array([0]).astype(int)
mptls = np.percentile(match_times, ptl_levels)
sptls = np.percentile(scan_times, ptl_levels)
blptls = np.percentile(bucket_lists, ptl_levels)
print("Match times (us) / Scan times (us)")
for (m, s, b, l) in zip(mptls, sptls, blptls, ptl_levels):
    print("%.1fth ptl: %f \t,\t %f \t,\t %d" % (l, m, s, b))
print("Average:", match_times.mean(), "\t,\t", scan_times.mean(), "\t,\t", bucket_lists.mean())




from stash import Stasher
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import os


datafile = sys.argv[1]
k = float(sys.argv[2])
data = np.loadtxt(datafile)
print(len(data))
target_dim = 1
mapped_dim = 0
target_nbuckets = 100

target_ptls = np.linspace(0, 100, target_nbuckets+1)
target_buckets = np.percentile(data[:,target_dim], target_ptls)
#target_buckets = np.linspace(data[:,target_dim].min(), data[:,target_dim].max(),
#        target_nbuckets+1)
target_buckets[-1] = np.nextafter(data[:,target_dim].max(), np.inf)
bucket_ids = np.digitize(data[:, target_dim], target_buckets[1:])

s = Stasher(data[:,mapped_dim], k, bucket_ids, mode='flattened')
print("Finished initializing Stasher, finding outliers")
outlier_ixs, stats = s.stash_outliers(10000)
uix = np.unique(outlier_ixs, return_counts=True)
print("Got %d outliers, %d unique" % (len(outlier_ixs), len(uix[0])))
print(uix[0][uix[1] > 1])
sys.exit(0)

plt.scatter(inpts[:, 0], inpts[:, 1], s=1, c='blue')
plt.scatter(outpts[:, 0], outpts[:, 1], s=1, c='red')

fname = os.path.splitext(os.path.basename(datafile))[0]
plt.savefig(fname + "_k%.1f.png" % k)
data = np.array(data)
np.savetxt(fname + "_opt_data_k%.1f.dat" % k, data)

plt.clf()
plt.plot(data[:, 0], data[:, 1], '-')
plt.savefig(fname + "_descent_k%.1f.png" % k)


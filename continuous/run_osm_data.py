from stash import Stasher
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import os
from bucketer import Schema

#datafile = "/home/ubuntu/data/airlines/flight_data_sample.bin"
datafile = "/home/ubuntu/learned-multi-index/gen/osm_dataset.bin"
data = np.fromfile(datafile, dtype=np.int64).reshape(-1, 9)
print("Loaded %d data points" % len(data))

target_specs = [Schema(2, bucket_width=10000)]
map_spec = Schema(3, bucket_width=10000)
# Each query goes over one bucket in the mapped dimension
k = 1

s = Stasher(data, map_spec, target_specs, k, alpha=0.5)
print("Finished initializing Stasher, finding outliers")
outlier_ixs, stats = s.stash_outliers_parallel(15, max_outliers_per_bucket=None)
uix = np.unique(outlier_ixs, return_counts=True)
print("Got %d outliers" % len(outlier_ixs))
print("True points: %d" % stats["total_true_points"])
print("Overhead: %d => %d with index" % (stats["initial_overhead"],
                                        stats["final_overhead_with_index"]))
s.write_mapping("osm/osm")

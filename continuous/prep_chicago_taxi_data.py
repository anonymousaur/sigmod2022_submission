import run_dataset as rdset
import gen_workload as gwl
import os
import numpy as np
from bucketer import Schema


mappings_0_2_8_p10000 = [(1, 0), (3,8), (4,8), (5,8)]
mappings_0_2_8_p1000 = [(1, 0), (3,8), (4,8), (5,8)]
mappings_primary_4 = [(1, 0), (2, 4), (3, 4), (5, 4), (8, 4)]

mappings = [mappings_0_2_8_p10000, mappings_0_2_8_p1000, mappings_primary_4]
suffixes = ["octree_0_2_8_p10000", "octree_0_2_8_p1000", "primary_4"]

for (maps, s) in zip(mappings[2:], suffixes[2:]):
    tbuckets = np.fromfile('chicago_taxi/target_buckets_%s.bin' % s, dtype=np.int32)
    for m in maps:
        map_dim = m[0]
        target_dim = m[1]
        args = {
            "name": "chicago_taxi",
            "datafile": "/home/ubuntu/correlations/continuous/chicago_taxi/chicago_taxi_sort_%s.bin" % s,
            "ncols": 9,
            "map_spec": Schema(map_dim, num_buckets=10000, mode='flattened'),
            "target_spec": [Schema(target_dim, bucket_ids = tbuckets)],
            "k": 1,
            "alphas": [-1, 0, 1, 2, 5, 10], #-1, 1, 5, 10, 15, 20, 30, 50],
            "suffix": s
        }
        rdset.run(args)



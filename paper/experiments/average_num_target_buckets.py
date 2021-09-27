import numpy as np
import os
import sys

mappings_file = sys.argv[1]
target_file = sys.argv[2]


def num_target_buckets(target_file):
    f = open(target_file)
    return int(next(f).split()[-1])

def get_all_mapped_buckets(mapfile):
    buckets = []
    started = False
    for line in open(mapfile):
        if line.startswith('mapping'):
            started = True
            continue
        if not started:
            continue
        buckets.append(len(line.split()) - 1)
    return buckets

t = num_target_buckets(target_file)
ms = get_all_mapped_buckets(mappings_file)

print("Avg mapped buckets: %f, as fraction of total = %f" % (np.mean(ms), np.mean(ms)/t))


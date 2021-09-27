import os
import sys
import numpy as np

filename = sys.argv[1]
start = False
buckets = []
print(filename)
nbuckets = 0
for line in open(filename):
    if line.startswith('source'):
        nbuckets = int(line.split()[-1])
    if (not start) and (not line.strip().startswith('mapping')):
        continue
    start = True
    buckets.append(len(line.split())-1)

print("Avg # buckets: %f" % (sum(buckets)/nbuckets))



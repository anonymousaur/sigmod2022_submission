import numpy as np
import sys

infile = sys.argv[1]
outfile = sys.argv[2]

buckets = []
for line in open(infile):
    p = line.strip().split(",")
    assert len(p) == 2
    s = int(p[0].strip())
    e = int(p[1].strip())
    buckets.append((s,e))

bixs = np.zeros(buckets[-1][1], dtype=np.int32)
for i, b in enumerate(buckets):
    bixs[b[0]:b[1]] = i

bixs.tofile(outfile)



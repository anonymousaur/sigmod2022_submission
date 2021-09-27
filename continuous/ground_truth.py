import numpy as np
from progress.bar import ShadyBar

datafile = "airline/airline.3_0.data"
qcol = 0
qfile = "airline/queries_0.dat"
outfile = "airline/ground_truth.dat"

data = np.fromfile(datafile, dtype=int).reshape(-1, 2)
data = data[:,0]

w = open(outfile, 'w')
b = ShadyBar(max = 1000)
for line in open(qfile):
    if line[0] == "=" or line.startswith("none"):
        continue
    assert line.startswith("ranges")
    parts = line.split()
    start = int(parts[1])
    end = int(parts[2])
    ct = np.logical_and(data >= start, data < end).sum()
    w.write(str(ct) + "\n")
    b.next()
b.finish()

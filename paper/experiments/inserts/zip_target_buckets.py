import os
import sys

ix_bounds_file = sys.argv[1]
val_bounds_file = sys.argv[2]

val_bounds = [float(x) for x in open(sys.argv[2]).read().splitlines()]
ix_bounds = open(sys.argv[1]).read().splitlines()

assert len(val_bounds) == len(ix_bounds)
for i, line in enumerate(ix_bounds):
    if i == 0:
        print(line.strip())
        continue
    print("%s %d %d" % (line, int(val_bounds[i-1]), int(val_bounds[i])))




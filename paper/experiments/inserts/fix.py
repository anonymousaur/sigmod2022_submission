import os
import sys
import numpy as np

def fix(f):
    base = os.path.splitext(os.path.basename(f))[0]
    base = base.replace('M_f', '_')
    base = base.replace('M_secondary_f', '_secondary_')
    rawfile = os.path.join('/home/ubuntu/correlations/paper/experiments/inserts/raw_results',
            base + ".output")
    times = []
    for line in open(rawfile):
        if line.startswith('Update time'):
            time_ms = line.split()[-1].strip().strip('ms')
            times.append(float(time_ms))
    newf = f.replace('.dat', '_updated.dat')
    g = open(newf, 'w')
    for line in open(f):
        g.write(line)
    g.write('Update time: %f' % float(np.mean(times)))
    g.close()

fix(os.path.abspath(sys.argv[1]))


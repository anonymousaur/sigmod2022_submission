import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as mplt
import numpy as np
import sys

def plt(pts, name):
    mplt.scatter(pts[:, 0], pts[:, 1], s=1, c='blue')
    mplt.savefig('%s.png' % name)
    mplt.xlim(-10, 20)
    mplt.scatter(pts[:, 0], pts[:, 1], s=1, c='blue')
    mplt.savefig('%s_fixed_window.png' % name)

if __name__ == '__main__':
    pts = np.fromfile(sys.argv[1], dtype=np.float64).reshape(-1, 2)
    plt(pts, sys.argv[2])


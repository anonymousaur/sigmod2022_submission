import numpy as np
import sys
import plot_data as pd


pareto_param = float(sys.argv[1]) if len(sys.argv) >= 2 else 4.0
fn = 'linear'

y = np.random.uniform(0, 10, 10000000)
s = np.random.uniform(-1, 1, len(y))
s[s > 0] = 1
s[s <= 0] = -1
x = None
if fn == 'linear':
    x = y + s * np.random.pareto(pareto_param, len(y))
elif fn == 'quadratic':
    x = np.sqrt(y) + s * np.random.pareto(pareto_param, len(y))
elif fn == 'cosine':
    x = np.cos(y/1.5)*10 + s * np.random.pareto(pareto_param, len(y))
else:
    print('unrecognized function %s' % fn)
    sys.exit(1)


pts = np.zeros((len(x), 2))
pts[:, 0] = x
pts[:, 1] = y
pts.astype(np.float64).tofile("%s.dat" % fn)

pd.plt(pts, fn)


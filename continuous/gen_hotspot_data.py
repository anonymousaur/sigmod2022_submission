import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def symmetric_pareto(n):
    s = np.random.uniform(-1, 1, n)
    s[s > 0] = 1
    s[s <= 0] = -1
    return s * np.random.pareto(4.0, n)


n = 9500000
x1 = -10 + symmetric_pareto(n)
x2 = 10 + symmetric_pareto(n)
y1 = -10 + symmetric_pareto(n)
y2 = 10 + symmetric_pareto(n)

x = np.concatenate((x1, x2))
y = np.concatenate((y1, y2))
#np.random.shuffle(x)
#np.random.shuffle(y)

xmin, xmax = x.min(), x.max()
ymin, ymax = y.min(), y.max()
background_x = np.random.uniform(xmin, xmax, 250000)
background_y = np.random.uniform(ymin, ymax, 250000)
x = np.concatenate((x, background_x))
y = np.concatenate((y, background_y))
pts = np.zeros((len(x), 2))
pts[:, 0] = x
pts[:, 1] = y
np.savetxt("hotspot.dat", pts)

#plt.scatter(x, y, s=1, c='blue')
#plt.savefig('hotspot.png')

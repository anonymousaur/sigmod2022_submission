import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

y = np.random.uniform(0, 10, 10000)
s = np.random.uniform(-1, 1, len(y))
s[s > 0] = 1
s[s <= 0] = -1
x = y + s * np.random.pareto(5.0, len(y))
pts = np.zeros((len(x), 2))
pts[:, 0] = x
pts[:, 1] = y

plt.scatter(pts[:, 0], pts[:, 1], s=1, c='blue')
plt.savefig('draw_linear.png')
for v in np.linspace(y.min(), y.max(), 11)[1:-1]:
    plt.axhline(v)
for v in np.linspace(x.min(), x.max(), 11)[1:-1]:
    plt.axvline(v)
plt.savefig('draw_linear_with_grid.png')


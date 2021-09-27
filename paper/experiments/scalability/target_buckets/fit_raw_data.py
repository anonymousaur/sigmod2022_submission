import numpy as np
from scipy.optimize import curve_fit
from scipy.stats import linregress

data = np.loadtxt('raw_data.txt')
x = data[:, :2]
y = data[:, 2]

def f(x, a, b):
    return a + b/(x[0]*x[1])

#p = curve_fit(f, x.T, y)
#a = p[0]
#b = p[1]
#
#ss_tot = np.sum((y - np.mean(y))**2)
#ss_res = np.sum((y - f(x.T, a, b))**2)
#print("r^2 = ", 1 - ss_res/ss_tot)

slope, icpt, r, _, _ = linregress(1 + 1/(x[:,0]*x[:,1]), y/x[:,1])
print(slope, icpt)
print(r)

xdata = 1 + 1/(x[:,0]*x[:,1])
xdata = xdata[:,np.newaxis]
ydata = y/x[:,1]
a, res, _, _ = np.linalg.lstsq(xdata, ydata)
var = np.sum((ydata - np.mean(ydata))**2)
print(a)
print(1 - res/var)


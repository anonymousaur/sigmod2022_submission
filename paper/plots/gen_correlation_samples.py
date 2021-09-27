import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np


# Weak functional correlation
data_weak = np.fromfile('/home/ubuntu/data/chicago_taxi/chicago_taxi_sample.bin',
        dtype=int).reshape(-1, 9)
plt.clf()
plt.gca().axes.get_xaxis().set_ticks([])
plt.gca().axes.get_yaxis().set_ticks([])
plt.scatter(data_weak[:,2], data_weak[:,4], s=0.01, c='blue')
plt.xlim(0,3600)
plt.ylim(0,7000)
plt.tight_layout()
plt.savefig('correlation_samples/weak_corr.png')

# Plot non-functional data
data_nf = data_weak
plt.clf()
plt.gca().axes.get_xaxis().set_ticks([])
plt.gca().axes.get_yaxis().set_ticks([])
plt.scatter(data_nf[:,8], data_nf[:,5], s=0.01, c='blue')
plt.xlim(0, 10000)
plt.ylim(0, 3000)
#plt.xlabel("Total Taxi Fare")
#plt.ylabel("Tip")
plt.tight_layout()
plt.savefig('correlation_samples/non_functional_corr.png')

# Plot multi-way correlation
plt.clf()
data_mw = data_nf
ax = plt.gcf().add_subplot(111, projection='3d')
ax.scatter(data_mw[:,2], data_mw[:,3], data_mw[:,4], s=0.01, alpha=0.5, c='blue')
ax.axes.get_xaxis().set_ticks([])
ax.axes.get_yaxis().set_ticks([])
ax.set_zticks([])
ax.set_xlim(0, 3600)
ax.set_ylim(0, 400)
ax.set_zlim(0, 7000)
ax.view_init(5, -58)
#ax.set_xlabel('Duration')
#ax.set_ylabel('Distance')
#ax.set_zlabel('Metered Fare')
plt.tight_layout()
plt.savefig('correlation_samples/multi_way_corr.png')


# Hotspot correlation
plt.clf()
data_hotspot = np.fromfile('/home/ubuntu/data/msft_buildings/msft_buildings_sample.bin',
        dtype=int).reshape(-1, 3)
plt.scatter(data_hotspot[:, 1], data_hotspot[:, 2], s=0.001, c='blue')
plt.gca().axes.get_xaxis().set_ticks([])
plt.gca().axes.get_yaxis().set_ticks([])
#plt.xlabel("Longtitude")
#plt.ylabel("Latitude")
plt.tight_layout()
plt.savefig('correlation_samples/hotspot_corr.png')



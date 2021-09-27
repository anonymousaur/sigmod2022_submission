from single_column_stasher import SingleColumnStasher
import numpy as np
import itertools
import sys

n = 20
k = 1
alpha = 2
counts = np.random.randint(0, 100, n)
print(counts)

def cost_for(counts, stashed_mask):
    unstashed = counts[np.logical_not(stashed_mask)]
    overhead = (unstashed.sum() - unstashed).sum()
    storage = alpha * len(counts) * (counts[stashed_mask].sum())
    return overhead + storage, overhead

def optimal_cost(counts):
    best_cost, best_mask = None, None
    for mask in itertools.product([False, True], repeat=len(counts)):
        npm = np.array(mask, dtype=bool)
        c, _ = cost_for(counts, npm)
        if best_cost is None or best_cost > c:
            best_cost, best_mask = c, npm
    c, s = cost_for(counts, best_mask)
    print(best_mask)
    return c, s, np.where(best_mask)[0]

scs = SingleColumnStasher(counts, k, alpha=alpha)
opt_cost, opt_overhead, opt_buckets_stashed = optimal_cost(counts)
print("Optimal cost:", opt_cost, ", scan overhead:", opt_overhead)
print("Optimal buckets stashed:", sorted(opt_buckets_stashed))
pts_stashed, buckets_stashed = scs.pop_all_sort()
cost, overhead = scs.cost()
print("Single Stasher cost:", cost, ", scan overhead:", overhead)
print("Buckets stashed:", sorted(buckets_stashed))




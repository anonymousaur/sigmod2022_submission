import numpy as np
import sys
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--dataset',
        type=str,
        required=True,
        help="Dataset filename")
parser.add_argument('--dim',
        type=int,
        required=True,
        help="Dimension of given dataset")
parser.add_argument('--query-col',
        type=int,
        required=True,
        help="The index of the column values to generate queries over")
parser.add_argument('--inflated-cols',
        type=int,
        required=True,
        help="Number of total columns in the final workload")
parser.add_argument('--sel',
        type=float,
        default=0.01,
        help="Average selectivity of queries")
parser.add_argument('--nqueries',
        type=int,
        required=True,
        help="Number of queries")
parser.add_argument('--output',
        type=str,
        required=True,
        help="File to write")
args = parser.parse_args()

class QueryGen1D(object):
    def __init__(self, data):
        self.values, self.counts = np.unique(data, return_counts=True)
        self.counts = np.cumsum(np.insert(self.counts, 0, 0))
        np.append(self.values, self.values[-1]+1)
        self.datasize = len(data)
    
    def try_gen(self, sel):
        tol = 1.1
        target = self.datasize * sel
        start_ix = np.random.randint(len(self.counts))
        start = self.counts[start_ix]
        l = start_ix+1
        r = len(self.counts)-1
        while l < r:
            mid = int((l+r)/2)
            if self.counts[mid] - start  > tol*target:
                r = mid
            elif self.counts[mid] - start < target/tol:
                l = mid+1
            else:
                return (self.values[start_ix], self.values[mid])
        if self.counts[l] - start < tol*target and self.counts[l] - start > target/tol:
            return (self.values[start_ix], self.values[l])
        return None

    def gen(self, sel):
        r = self.try_gen(sel)
        while r is None:
            sys.stdout.write('x')
            r = self.try_gen(sel)
        sys.stdout.write('.')
        sys.stdout.flush()
        return r

# Write a query on col with range r to file f
def write(f, r, col):
    s = "==========\n"
    for d in range(args.inflated_cols):
        if d == col:
            s += "ranges %s %s\n" % (str(r[0]), str(r[1]))
        else:
            s += "none\n"
    f.write(s)


data = np.fromfile(args.dataset, dtype=int).reshape(-1, args.dim)
qg = QueryGen1D(data[:, args.query_col])
print("Initialized QueryGen")

f = open(args.output, 'w')
for i in range(args.nqueries):
    ix = np.random.randint(args.inflated_cols - args.dim + 1)
    if ix == 0:
        ix = args.query_col
    else:
        ix += args.dim-1
    write(f, qg.gen(args.sel), ix)
f.close()


import numpy as np
import argparse
import random
import sys

parser = argparse.ArgumentParser("")
parser.add_argument("--dataset",
        type=str,
        required=True,
        help="Path to binary data file")
parser.add_argument("--dim",
        type=int,
        required=True,
        help="Dimension of the dataset")
parser.add_argument("--filter-dims",
        type=int,
        nargs='+',
        help="Dimensions to create ranges on, chooses uniformly between each")
parser.add_argument("--num-queries",
        type=int,
        default=1000,
        help="number of queries to generate")
parser.add_argument("--output",
        type=str,
        required=True,
        help="file to write output to")
args = parser.parse_args()

assert len(args.filter_dims) > 0, "Requires at least one --filter-dim"

class MeasureBetaQueryGen(object):
    def __init__(self, dataset, dim):
        self.data = np.fromfile(dataset, dtype=np.int64).reshape(-1, dim)
        print("Loaded dataset")
        self.ptls = {}
        default_div = 10000
        for d in args.filter_dims:
            vals = np.sort(self.data[:,d])
            ixs = np.linspace(0, len(vals)-1, default_div).astype(int) 
            self.ptls[d] = np.unique(vals[ixs])
            print("Loaded percentiles for dim %d" % d)

    def query_str(self, ranges):
        s = "==========\n"
        for d in range(args.dim):
            if d in ranges:
                s += "ranges %s %s\n" % (str(ranges[d][0]), str(ranges[d][1]))
            else:
                s += "none\n"
        return s

    def gen_query(self, dims):
        q = {}
        for d in dims:
            div = len(self.ptls[d]) - 1
            start = random.random() * div
            width = random.uniform(0, 0.1) * div + 1
            start = int(start)
            end = min(int(start + width), div)
            q[d] = (self.ptls[d][start], self.ptls[d][end])
        return q

if __name__ == "__main__":
    w = open(args.output, 'w')
    mb = MeasureBetaQueryGen(args.dataset, args.dim)
    for _ in range(args.num_queries):
        q = mb.gen_query(args.filter_dims)
        w.write(mb.query_str(q))
        sys.stdout.write('.')
        sys.stdout.flush()
    w.close()


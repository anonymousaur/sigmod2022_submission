import numpy as np
import argparse
import random
import sys
import math
from bucketer import Schema

# Generates queries whose distribution matches that of the points.


class Hist2D(object):
    # data has 2 dimensions
    def __init__(self, data):
        max_buckets = 10000
        ptls = np.linspace(0, 100, max_buckets+1)
        bnds0 = np.unique(np.percentile(data[:,0], ptls, interpolation='nearest'))
        bnds1 = np.unique(np.percentile(data[:,1], ptls, interpolation='nearest'))
        print("Creating histogram")
        self.cumul_hist = self.cuml_hist(data, bnds0, bnds1)
        self.data_size = len(data)
        print("Done")

    # Returns an histogram where each cell (i,j) contains the number of points in
    # cells (a,b) such that a <= i and b <= j
    def cuml_hist(self, data, bnds1, bnds2):
        hist = np.histogram2d(data[:,0], data[:,1], (bnds1, bnds2))
        self.points_present = hist[0] > 0
        self.hist = np.zeros((hist[0].shape[0]+1, hist[0].shape[1]+1))
        self.hist[1:, 1:] = hist[0]
        self.x_count = self.hist.sum(axis=1)
        self.y_count = self.hist.sum(axis=0)
        self.x_cumul = np.cumsum(self.x_count)
        self.y_cumul = np.cumsum(self.y_count)
        self.bounds = (hist[1], hist[2])
        return np.cumsum(np.cumsum(self.hist, axis=0), axis=1)

    def custom_ptl(self, data, ptls):
        ddata = np.sort(data)
        vals = []
        for p in ptls:
            ix = int(p * len(data)/100.)
            if ix >= len(data):
                vals.append(data[-1]+1)
                break
            if len(vals) > 0 and data[ix] <= vals[-1]:
                continue
            while ix > 0 and data[ix] == data[ix-1]:
                ix += 1
            vals.append(data[ix])
        return vals

    # Gets all points strictly less than this index
    def get(self,x, y):
        return self.cumul_hist[x, y]

    # The arguments are indices to the bounds array
    def points_within(self, x1, y1, x2, y2):
        return self.get(x2, y2) + self.get(x1, y1) - self.get(x1, y2) - self.get(x2, y1)

    def get_y_bucket(self, x_start, y_start, x_bucket, direction):
        xstart_val = self.bounds[0][x_start]
        ystart_val = self.bounds[1][y_start]
        xval = self.bounds[0][x_bucket]
        yval = (xval - xstart_val)* np.tan(direction) + ystart_val 
        y_bucket = np.searchsorted(self.bounds[1], yval, side="right")
        return max(y_bucket, y_start+1)

    def random_range_for_sel(self, cum_counts, target, target_high=None):
        max_ix = np.searchsorted(cum_counts, cum_counts[-1] - target)
        start = random.randint(0, max_ix)
        l = start
        r = len(cum_counts)-1
        while l < r:
            mid = int((l+r)/2)
            pts = cum_counts[mid] - cum_counts[start]
            if pts < target:
                l = mid+1
            elif target_high is None or pts > target_high:
                r = mid
            else:
                return (start, mid)
        if cum_counts[l] - cum_counts[start] < target:
            return None
        return (start, l)

    def try_query_with_target(self, selectivity):
        single_dim_sel = math.sqrt(selectivity)
        xsel = random.uniform(selectivity, single_dim_sel)
        ysel = random.uniform(selectivity, single_dim_sel)
        x_range = self.random_range_for_sel(self.x_cumul, xsel * self.data_size)
        if x_range is None:
            print("Bad x range")
            return None, None
        # Sum of values in this x-range, accumulated over each y bucket.
        y_cum_counts = self.cumul_hist[x_range[1]] - self.cumul_hist[x_range[0]]
        target_pts = selectivity * self.data_size
        # Find the y range that gets us the required number of points.
        y_range = self.random_range_for_sel(y_cum_counts, 0.5*target_pts,
                target_high=2*target_pts)
        if y_range is None:
            print("Bad y range")
            return None, None
        actual_y_sel = (self.y_cumul[y_range[1]] - self.y_cumul[y_range[0]]) / self.data_size
        if actual_y_sel < selectivity or actual_y_sel > single_dim_sel:
            print("Couldn't find good y selectivity range")
            return None, None
        print(self.x_count[x_range[0]:x_range[1]].sum() / self.data_size, '\t',
                self.y_count[y_range[0]:y_range[1]].sum() / self.data_size)
        pts_in_rect = self.points_within(x_range[0], y_range[0], x_range[1], y_range[1])
        assert pts_in_rect < selectivity*self.data_size*0.5 and \
                pts_in_rect > selectivity*self.data_size*2, "Got %d pts (sel %f)" % \
                (pts_in_rect, pts_in_rect / self.data_size)
        return ((self.bounds[0][x_range[0]], self.bounds[0][x_range[1]]),\
                (self.bounds[1][y_range[0]], self.bounds[1][y_range[1]])), pts_in_rect
         

    # This might not produce a query (by chance)
    def try_query_with_target_old(self, target_bounds):
        xstart = random.randint(0, self.cumul_hist.shape[0]-1)
        present_buckets = np.where(self.points_present[xstart])[0]
        ystart = random.randint(present_buckets.min() - 1, present_buckets.max() + 1)
        # Choose a uniform angle between 10 and 80 degrees.
        direction = random.uniform(15 * np.pi/180., 75 * np.pi / 180.)
        # Search along this direction until we're within the bound of target
        l = xstart+1
        r = self.cumul_hist.shape[0]
        start_ct = self.get(xstart, ystart)
        while (l < r):
            mid_x = int((l + r)/2)
            mid_y = self.get_y_bucket(xstart, ystart, mid_x, direction)
            mid_y = min(mid_y, self.cumul_hist.shape[1])
            pts_in_rect = self.points_within(xstart, ystart, mid_x, mid_y)
            if pts_in_rect < target_bounds[0]:
                l = mid_x+1
            elif pts_in_rect > target_bounds[1]:
                r = mid_x
            else:
                print(self.x_count[xstart:mid_x].sum() / self.data_size, '\t',
                        self.y_count[ystart:mid_y].sum() / self.data_size)
                return ((self.bounds[0][xstart], self.bounds[0][mid_x]),
                        (self.bounds[1][ystart], self.bounds[1][mid_y])), pts_in_rect
    
        final_x = int(l)
        final_y = self.get_y_bucket(xstart, ystart, final_x, direction)
        final_y = min(final_y, self.cumul_hist.shape[1])
        pts_in_rect = self.points_within(xstart, ystart, final_x, final_y)
        if pts_in_rect < target_bounds[0] or pts_in_rect > target_bounds[1]:
            return None, None
        return ((self.bounds[0][xstart], self.bounds[0][final_x]),
                (self.bounds[1][ystart], self.bounds[1][final_y])), pts_in_rect




class QueryGen(object):
    def __init__(self, dataset, dtype="int"):
        self.data = dataset
        self.dtype = dtype
        self.dim = self.data.shape[1]
    
    def get_range(self, pt, dims, widths):
        ranges = {}
        for d, w in zip(dims, widths):
            ranges[d] = tuple(np.array([pt[d]-w/2, pt[d]+w/2]).astype(self.dtype))
        return ranges

    def query_str(self, ranges):
        s = "==========\n"
        for d in range(self.dim):
            if d in ranges:
                s += "ranges %s %s\n" % (str(ranges[d][0]), str(ranges[d][1]))
            else:
                s += "none\n"
        return s

    def gen_proportional(self, dims, widths, n, outfile):
        print("Generating proportional queries...")
        ixs = np.random.randint(len(self.data), size=n)
        pts = self.data[ixs,:]
        f = open(outfile, 'w')
        for i in range(n):
            r = self.get_range(pts[i], dims, widths)
            f.write(self.query_str(r))
        f.close()

    def gen_uniform(self, dims, widths, n, outfile):
        print("Generating uniform queries...")
        maxes = []
        bucket_hashes = np.zeros(len(self.data))
        for d, w in zip(dims, widths):
            vals = self.data[:, d]
            b = np.floor((vals - vals.min())/(w/2)).astype(int)
            maxes.append(b.max()+1)
            bucket_hashes = (b.max()+1)*bucket_hashes + b
        
        # First, select a bucket uniformly at random. Then select a starting point within that
        # bucket uniformly at random.
        unique_buckets = np.unique(bucket_hashes)
        bucket_sample_ixs = np.random.choice(len(unique_buckets), size=n, replace=True)
        bucket_sample = unique_buckets[bucket_sample_ixs]
        bucket_coords = np.zeros((len(bucket_sample), len(dims)))
        for i, m in enumerate(reversed(maxes)):
            bucket_coords[:, -i-1] = bucket_sample % m
            bucket_sample /= m
        offsets = np.zeros((len(dims), n))
        pts = np.zeros((n, self.dim))
        for i, (d, w) in enumerate(zip(dims, widths)):
            offsets = np.random.uniform(0, w/2, size=len(bucket_sample))
            pts[:, d] = bucket_coords[:, i]*w/2 + self.data[:,d].min() + offsets[i]
        f = open(outfile, 'w')
        for i in range(n):
            r = self.get_range(pts[i], dims, widths)
            f.write(self.query_str(r))
        f.close()

    def find_range_end(self, cumuls, startix, target_bounds):
        left = startix
        right = len(cumuls)-1
        base_count = cumuls[startix-1] if startix > 0 else 0
        c = cumuls[startix] - base_count
        while left < right:
            mid = int((left + right)/2)
            c = cumuls[mid] - base_count
            if c < target_bounds[0]:
                left = mid+1
            elif c > target_bounds[1]:
                right = mid
            else:
                # We return an inclusive range
                return mid, c
        return left, cumuls[left] - base_count
    
    def gen_equiselective_1D(self, dim, sel, n, outfile):
        vals, counts = np.unique(self.data[:, dim], return_counts=True)
        target = int(sel * len(self.data))
        max_ix = len(counts)
        s = 0
        for c in reversed(counts):
            s += c
            if s >= target:
                break
            max_ix -= 1
        cumuls = np.cumsum(counts)
        f = open(outfile, 'w')
        written = 0
        counts = []
        while written < n:
            startix = random.randint(0, max_ix)
            endix, c = self.find_range_end(cumuls, startix, (0.9*target, 1.1*target)) 
            if c < 0.5*target or c > 2*target:
                # Too far out of bounds
                sys.stdout.write('x')
                continue
            sys.stdout.write('.')
            sys.stdout.flush()
            counts.append(c)
            r = { dim: (vals[startix], vals[endix]) }
            f.write(self.query_str(r))
            written += 1
        print("\nFinished generating queries:")
        f.close()
        ptls = [0, 10, 50, 90, 100]
        count_ptls = np.percentile(counts, ptls)
        for (p, c) in zip(ptls, count_ptls):
            print("%d ptl: %d" % (p, c))

    def gen_equiselective_2D(self, dims, sel, n, outfile):
        assert len(dims) == 2
        h = Hist2D(self.data[:,(dims[0], dims[1])])
        target = int(sel * len(self.data))
        written = 0
        counts = []
        f = open(outfile, 'w')
        while written < n:
            res, ct = h.try_query_with_target(sel)
            if res is None:
                sys.stdout.write('x')
                sys.stdout.flush()
                continue
            r = {
                    dims[0]: res[0],
                    dims[1]: res[1]
                }
            sys.stdout.write('.')
            sys.stdout.flush()
            counts.append(ct)
            f.write(self.query_str(r))
            written += 1
        f.close()
        ptls = [0, 10, 50, 90, 100]
        count_ptls = np.percentile(counts, ptls)
        for (p, c) in zip(ptls, count_ptls):
            print("%d ptl: %d" % (p, c))

                
    def gen_equiselective(self, dims, sels, n, outfile):
        assert len(dims) <= 2 and len(sels) == 1
        if len(dims) == 1:
            self.gen_equiselective_1D(dims[0], sels[0], n, outfile)
        if len(dims) == 2:
            self.gen_equiselective_2D(dims, sels[0], n, outfile)

def gen_from_spec(args, distribution, nq, output):
    dataset = np.fromfile(args["datafile"], dtype=int).reshape(-1, args["ncols"])
    cols = [args["map_dims"][0]] + [td[0] for td in args["target_dims"]]
    dataset = dataset[:,cols]
    qg = QueryGen(dataset)
    filter_dims = [0]
    filter_widths = [args["map_dims"][1]] if distribution != "point" else [2]
    if distribution == "uniform":
        qg.gen_uniform(filter_dims, filter_widths, nq, output)
    else:
        qg.gen_proportional(filter_dims, filter_widths, nq, output)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("QueryGen")
    parser.add_argument("--dataset",
            type=str,
            required=True,
            help="Pointer to the dataset to generate relevant queries")
    parser.add_argument("--dim",
            type=int,
            required=True,
            help="Dimension of the dataset")
    parser.add_argument("--dtype",
            type=str,
            required=True,
            help="dtype of the dataset")
    parser.add_argument("--nqueries",
            type=int,
            required=True,
            help="Number of queries to generate")
    parser.add_argument("--filter-dims",
            type=int,
            nargs="+",
            required=True,
            help="Dimensions to generate queries for")
    parser.add_argument("--filter-widths",
            type=float,
            nargs="+",
            help="Widths along each dimension (len(--type) == len(--width))." + \
                    " If this and --selectivies aren't set, generates widths uniformly." + \
                    " Used when --distribution={uniform, proportional}")
    parser.add_argument("--selectivities",
            type=float,
            nargs="+",
            help="Selectivities along each dimension (between 0 and 1)," + \
                    " used when --distribution=equiselective")
    parser.add_argument("--distribution",
            type=str,
            choices=["uniform", "proportional", "equiselective"],
            required=True,
            help="Distribution of queries. If proportional, will generate queries proportional to " + \
                "the distribution of points along the mapped dimension. Otherwise, queries will be " + \
                "uniform along the mapped dimension wherever there are points.")
    parser.add_argument("--output",
            type=str,
            required=True,
            help="File to write queries to")
    
    args = parser.parse_args()
    dataset = np.fromfile(args.dataset, dtype=args.dtype).reshape(-1, args.dim)
    qg = QueryGen(dataset)
    assert (len(args.filter_dims) > 0)
    assert (args.filter_widths is None or args.selectivities is None)
    
    if args.distribution == "proportional":
        qg.gen_proportional(args.filter_dims, args.filter_widths, args.nqueries, args.output)
    elif args.distribution == "uniform":
        qg.gen_uniform(args.filter_dims, args.filter_widths, args.nqueries, args.output)
    elif args.distribution == "equiselective":
        qg.gen_equiselective(args.filter_dims, args.selectivities, args.nqueries, args.output)





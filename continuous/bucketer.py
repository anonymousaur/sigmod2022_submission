import numpy as np
import time
from gen_1d_buckets import *

class Schema(object):
    def __init__(self, dim, num_buckets=None, bucket_width=None, bucket_ids=None, categorical=False, mode='flattened'):
        valid = int(num_buckets is not None)
        valid += int(bucket_width is not None)
        valid += int(categorical)
        valid += int(bucket_ids is not None)
        assert valid == 1, \
                "Exactly one of num_buckets, bucket_width, bucket_ids, and categorical can be specified"
        self.dim = dim
        self.num_buckets = num_buckets
        self.bucket_width = bucket_width
        self.categorical = categorical
        self.mode = mode
        self.bucket_ids = bucket_ids

    # Given some bucket ids, if there are gaps (e.g., there is no point with bucket 10), shift the
    # bucket IDs so there are no gaps
    def sequentialize(self, buckets, ranges):
        start = time.time()
        _, inv = np.unique(buckets, return_inverse=True)
        end = time.time()
        print("Sequentializing took %.02fs" % (end-start))
        return inv

    def custom_percentile(self, data, ptls):
        sort_ix = np.argsort(data)
        sorted_data = data[sort_ix]
        breaks = []
        vals = []
        for p in ptls:
            ix = int(p * len(sorted_data) / 100)
            if len(breaks) > 0 and ix <= breaks[-1]:
                # This will just add a redundant bucket
                continue
            if ix >= len(data):
                breaks.append(len(data))
                vals.append(sorted_data[-1]+1)
                break
            while ix > 0 and sorted_data[ix] == sorted_data[ix-1]:
                ix += 1
                if ix == len(sorted_data):
                    break
            breaks.append(ix)
            if ix == len(sorted_data):
                vals.append(sorted_data[-1]+1)
                break
            vals.append(sorted_data[ix])
        buckets = np.zeros(len(data), dtype=int)
        assert breaks[0] == 0
        prev = 0
        for i, v in enumerate(breaks[1:]):
            buckets[sort_ix[prev:v]] = i
            prev = v
        return buckets, vals

    def bucketize_with_nbuckets(self, data, sequentialize=False):
        print("Bucketizing 1d buckets")
        buckets = gen_1d_buckets(data, self.num_buckets)
        #if self.mode == 'flattened':
        #    ptls = np.linspace(0, 100, self.num_buckets+1)
        #    buckets = np.percentile(data, ptls, interpolation='nearest')
        #    # Get rid of duplicates if there are a lot of points with a single value.
        #    print("Found bucket bounds, uniquifying")
        #    buckets = np.unique(buckets)
        #    #digitized, buckets = self.custom_percentile(data, ptls)
        #elif self.mode == 'equispaced':
        #    buckets = np.linspace(data.min(), data.max(), self.num_buckets+1)
        #else:
        #    print("Unrecognized mode %s" % mode)
        #buckets[-1] = data.max()+1
        #print("Got unique bucket bounds")
        bounds = np.vstack((np.arange(len(buckets)-1).astype(int), buckets[:-1], buckets[1:])).T
        print("Found bucket bounds, digitizing...")
        digitized = np.digitize(data, buckets[1:], right=False)
        return digitized, bounds

    def bucketize_with_width(self, data, sequentialize=False):
        b = np.floor(data/self.bucket_width)
        bmin, bmax = b.min(), b.max()
        b = b.astype(int)
        buq, inv = np.unique(b, return_inverse=True)
        bounds = np.vstack((buq, buq * self.bucket_width, (buq+1)*self.bucket_width)).T
        assert (bounds.shape == (len(buq), 3))
        if sequentialize:
            b = inv
            bounds[:,0] = np.arange(len(buq)).astype(int)
        return b, bounds

    def bucketize(self, data, sequentialize=False):
        buckets = None
        bounds = None
        if self.bucket_width is not None:
            buckets, bounds = self.bucketize_with_width(data, sequentialize)
        elif self.num_buckets is not None:
            buckets, bounds = self.bucketize_with_nbuckets(data, sequentialize)
            print ("Max bucket:", buckets.max())
        elif self.bucket_ids is not None:
            buckets, bounds = self.bucket_ids, None
        elif self.categorical:
            buckets, bounds = data, data
        return buckets, bounds
    
class Bucketer(object):
    # Spec is a list of Schema objects
    def __init__(self, spec, data, mode='flattened', sequentialize=False):
        self.spec = spec
        self.ids = None
        self.ranges = None
        self.bucket(data, sequentialize)

    def bucket(self, data, sequentialize=False):
        ranges = []
        ids = np.zeros((len(data), len(self.spec)), dtype=int)
        for j, s in enumerate(self.spec):
            ids[:,j], r = s.bucketize(data[:,s.dim], sequentialize=sequentialize)
            ranges.append(r)
        folded_ids = np.zeros(ids.shape[0], dtype=int)
        scale = 1
        for j in range(ids.shape[1]):
            folded_ids += ids[:,j] * scale
            scale *= ids[:,j].max()+1
        self.ids = folded_ids
        self.ranges = ranges
        return folded_ids, ranges

    def get_ids(self):
        return self.ids

    def get_ranges(self):
        for r in self.ranges:
            assert r is not None
        return self.ranges

    def dims(self):
        return [s.dim for s in self.spec] 

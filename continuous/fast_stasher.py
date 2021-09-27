# Stashes points into the outlier buffer if they decrease scan overhead.

import numpy as np
import sys
import os
import time
import math 
import continuous_correlation_map as ccm
from grid_stasher import GridStasher
from single_column_stasher import SingleColumnStasher 
from progress.bar import ShadyBar
from multiprocessing import Pool
from collections import defaultdict
from bucketer import Bucketer

OUTLIER_BUCKET = 1 << 50

class Stasher(object):
    # pts is a numpy array of shape n x 2. The 0th column is the mapped dimension, and the 1st
    # column is the target dimension.
    # k is the average length of the ranges on the mapped dimension.
    # buckets is an integer list of bucket ids for each point (size = (len(pts),))
    def __init__(self, data, mapped_schema, target_schemas, beta=6):
        self.using_display = True
        map_bucketer = Bucketer([mapped_schema], data, sequentialize=True)
        mapped_buckets = map_bucketer.get_ids()
        self.max_mapped_bucket = mapped_buckets.max() + 1
        print("Got all mapped bucket IDs")
        target_bucketer = Bucketer(target_schemas, data)
        target_buckets = target_bucketer.get_ids()
        print("Got all target bucket IDs")
        # Make sure data is already sorted in order of the target bucket value.
        assert np.all(np.diff(target_buckets) >= 0)
        self.beta = beta

        # Keeps number of points and mapped buckets (in that order)
        self.target_bucket_counts = defaultdict(lambda: (0,0))
        self.num_pts = len(target_buckets)
        self.max_target_bucket = target_buckets.max()
        unique_cells = defaultdict(list)
        for mb, tb, ct in self.unique_cells(mapped_buckets, target_buckets, return_ids=False):
            p = self.target_bucket_counts[tb]
            self.target_bucket_counts[tb] = (p[0]+ct, p[1]+1)
        self.orig_scan_overhead = sum(ps * ms for (ps, ms) in self.target_bucket_counts.values())
        unique_cells = sum(ms for (_, ms) in self.target_bucket_counts.values())
        print("Got %d target buckets" % len(self.target_bucket_counts))
        print("Found %d unique cells" % unique_cells)

        self.target_bucket_ids = target_buckets
        self.mapped_bucket_ids = mapped_buckets

        self.num_mapped_buckets = mapped_buckets.max()+1
        self.data = data
        self.outlier_indexes = []
        self.mapped_dim = map_bucketer.spec[0].dim
        self.target_dims = [s.dim for s in target_bucketer.spec]
        print("Mapped dim = %d, target_dims = %s" % (self.mapped_dim, str(self.target_dims)))

        self.ccm = ccm.ContinuousCorrelationMap()
        self.ccm.set_mapped_bucketer(map_bucketer)
        self.ccm.set_target_bucketer(target_bucketer)
        print("Done finding bucket IDs")
        
        self.bucketmap = defaultdict(list)

    def target_buckets(self, target_bucket_ids):
        start = 0
        initial_id = target_bucket_ids[start]
        ix = start
        while start < len(target_bucket_ids):
            initial_id = target_bucket_ids[start]
            while ix < len(target_bucket_ids) and target_bucket_ids[ix] == initial_id:
                ix += 1
            yield initial_id, start, ix
            start = ix


    def unique_cells(self, mapped_bucket_ids, target_bucket_ids, return_ids=False):
        for tb, start, end in self.target_buckets(target_bucket_ids):
            buckets = defaultdict(list) if return_ids else defaultdict(int)
            for ix in range(start, end):
                if return_ids:
                    buckets[mapped_bucket_ids[ix]].append(ix)
                else:
                    buckets[mapped_bucket_ids[ix]] += 1
            for mb, ixs in buckets.items():
                yield (mb, tb, ixs)

    def to_hash(self, m, t):
        return t * self.max_mapped_bucket + m

    def from_hash(self, h):
        t = int(h / self.max_mapped_bucket)
        m = h % self.max_mapped_bucket
        return m, t

    def display(self, d):
        self.using_display = d

    # In the revised model, the host bucket isn't modified 
    def stash_outliers_fast(self, alpha, max_outliers=None):
        cuml_outliers = []
        cuml_stats = defaultdict(float)
        inlier_cells = []
        outlier_indexes = []


        scan_overhead = self.orig_scan_overhead
        orig_cost = self.cost(alpha, scan_overhead) 
        cuml_stats["initial_cost"] = orig_cost
        cuml_stats["initial_scan_overhead"] = self.orig_scan_overhead
        inc = 10000
        #b = ShadyBar(max=len(self.unique_cells)/inc)
        processed = 0
        # Because we're not sorting the buckets, we can't be optimal unless either we don't stash
        # anything, or we don't have a limit.
        assert max_outliers is None or max_outliers == 0 
        max_outliers = self.num_pts if max_outliers is None else max_outliers
        bucketmap = defaultdict(list)
        for m, t, ixs in self.unique_cells(self.mapped_bucket_ids, self.target_bucket_ids,
                return_ids=True):
            if len(ixs) + len(outlier_indexes) < max_outliers and \
                    len(ixs) * (self.beta + alpha * self.orig_scan_overhead / self.num_pts) < \
                        self.target_bucket_counts[t][0]:
            #if len(ixs) + len(outlier_indexes) < max_outliers and \
            #        len(ixs) * self.beta * (alpha + 1) < self.target_bucket_counts[t][0]:
                outlier_indexes.extend(ixs)
                scan_overhead -= self.target_bucket_counts[t][0]
            else:
                bucketmap[m].append(t)
            processed += 1
            #if processed % inc == 0:
            #    b.next()
        #b.finish()
        self.outlier_indexes = outlier_indexes

        cuml_stats["data_size"] = len(self.data)
        cuml_stats["final_cost"] = self.cost(alpha, scan_overhead)
        cuml_stats["final_scan_overhead"] = scan_overhead
        cuml_stats["num_outliers"] = len(outlier_indexes)
        # Indices in the original dataset
        self.ccm.set_bucket_map(bucketmap)
        return self.outlier_indexes, cuml_stats

    # If we are already given the outlier list (a list of indexes into the data), determine which
    # target buckets to keep in the mapping and which can be removed.
    def stash_given_outliers(self, outlier_list):
        outliers = set(outlier_list)
        targets_to_remove = []
        bucketmap = defaultdict(list)
        for m, t, ixs in self.unique_cells(self.mapped_bucket_ids, self.target_bucket_ids,
                return_ids=True):
            ct = 0
            for ix in ixs:
                if ix not in outliers:
                    break
                ct += 1
            if ct == len(ixs):
                targets_to_remove.append((m,t))
            else:
                bucketmap[m].append(t)

        self.ccm.set_bucket_map(bucketmap)
        self.outlier_indexes = outlier_list

    def cost(self, alpha, scan_overhead):
        return (scan_overhead + self.beta * len(self.outlier_indexes))/self.orig_scan_overhead + \
                alpha * len(self.outlier_indexes) / self.num_pts

    def write_mapping(self, prefix):
        dirname = os.path.dirname(prefix)
        if not os.path.isdir(dirname):
            os.makedirs(dirname)
            print('Created output directory', dirname)

        rel_dims = [self.mapped_dim]
        rel_dims.extend(self.target_dims)
        # Mapped dimension is always the 0th column in the new datset. The target dimensions are in
        # order of their schema after that.
        
        mapping_file = prefix + ".mapping"
        f = open(mapping_file, 'w')
        self.ccm.write(f)
        f.close()

        ntarget_buckets = self.max_target_bucket + 1

        # Dump information for the indexer
        outlier_indexes_file = prefix + ".outliers"
        target_bucket_file = prefix + ".targets"
        
        # These are the outlier indexes in the original dataset
        np.array(self.outlier_indexes, dtype=int).tofile(outlier_indexes_file)
        
        # Write the mapping from target bucket to physical index range in the sorted data 
        fobj = open(target_bucket_file, 'w')
        fobj.write("target_index_ranges\t%s\t%d\n" % ('_'.join(str(t) for t in self.target_dims),
            len(self.target_bucket_counts)))
        prev = 0
        keys = sorted(self.target_bucket_counts.keys())
        for k in keys:
            c, _ = self.target_bucket_counts[k]
            fobj.write("%d\t%d\t%d\n" % (k, prev, prev+c))
            prev += c
        assert prev == self.num_pts

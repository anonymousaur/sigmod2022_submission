# Stashes points into the outlier buffer if they decrease scan overhead.

import numpy as np
import sys
import os
import time
import math 
import continuous_correlation_map as ccm
from grid_stasher import GridStasher
from single_column_stasher import SingleColumnStasher 

class Stasher(object):
    # pts is a numpy array of shape n x 2. The 0th column is the mapped dimension, and the 1st
    # column is the target dimension.
    # k is the average length of the ranges on the mapped dimension.
    # buckets is an integer list of bucket ids for each point (size = (len(pts),))
    def __init__(self, pts, k, buckets, mode='flattened'):
        self.maxs, self.mins = np.max(pts, axis=0), np.min(pts, axis=0)
        self.pts = pts
        self.nbuckets_mapped = 100
        self.nbuckets_mapped = min(self.nbuckets_mapped, len(np.unique(pts)))
        kr = math.ceil(self.nbuckets_mapped * k / (self.pts.max() - self.pts.min()))
        self.kr = min(self.nbuckets_mapped, kr)
        
        print("Bucketizing")
        self.buckets = self.bucketize(buckets, mode)
        self.overhead_list = [[] for _ in range(len(self.buckets))]

    def bucketize(self, bucket_ids, mode='flattened'):
        # Bucket along the target dimension as the primary key. Secondary key is the mapped dimension
        # value.
        mmax, mmin = self.pts.max(), self.pts.min()
        if mode == 'flattened':
            mapped_ptls = np.linspace(0, 100, self.nbuckets_mapped+1)
            self.mapped_buckets = np.percentile(self.pts, mapped_ptls)
        elif mode == 'equispaced':
            self.mapped_buckets = np.linspace(mmin, mmax, self.nbuckets_mapped+1)
        else:
            print("Mode %s not recognized" % mode)
            sys.exit(1)
        self.mapped_buckets[-1] = np.nextafter(mmax, mmax+1)
       
        # Sort by bucket id then by mapped value
        sortkey = bucket_ids*(mmax - mmin + 1) + self.pts
        self.sort_ixs = np.argsort(sortkey)
        bucket_ids = bucket_ids[self.sort_ixs]
        self.pts = self.pts[self.sort_ixs]

        buckets = [(0,0)] * bucket_ids[-1]
        end = 0
        for i in range(bucket_ids[-1]):
            start = end
            end = np.searchsorted(bucket_ids, i, side='right')
            buckets[i] = (start, end)

        return buckets

        #target_ptls = np.linspace(0, 100, self.nbuckets_target+1)
        #self.target_buckets = np.percentile(colt, target_ptls)
        ##self.target_buckets = np.linspace(colt.min(), colt_max, self.nbuckets_target+1)
        #self.target_buckets[-1] = np.nextafter(colt_max, colt_max+10)
        #print(self.mapped_buckets)
        #print(self.target_buckets)
        #sortkey[:, 1] = np.searchsorted(self.target_buckets, colt, side='right') - 1
        #sortkey[:, 0] = np.searchsorted(self.mapped_buckets, colm, side='right') - 1
        #sortkey[:,1] = (self.nbuckets_target * 0.999 * \
        #        (colt - self.mins[1]) / (self.maxs[1] - self.mins[1])).astype(int)
        #sortkey[:,0] = (self.nbuckets_mapped * 0.999 * \
        #        (colm - self.mins[0]) / (self.maxs[0] - self.mins[0])).astype(int)

    def buckethash(self, row, col):
        return row * self.nbuckets_mapped + col

    # Uses the single column stasher.
    def stash_outliers(self, max_pts_to_stash):
        stash_size = 0
        initial_overhead = 0
        final_overhead = 0
        total_true_pts = 0
        outlier_ixs = []
        for b in self.buckets:
            col_pts = self.pts[b[0]:b[1]]
            counts = np.histogram(col_pts, bins=self.mapped_buckets)[0]
            scs = SingleColumnStasher(counts, self.kr)
            initial_so, true_pts = scs.scan_overhead()
            total_true_pts += true_pts
            initial_overhead += initial_so
            while True:
                s, _, ixs = scs.pop_best()
                if s is None:
                    break
                stash_size += s
                start = np.searchsorted(col_pts, self.mapped_buckets[ixs[0]]) + b[0]
                end = np.searchsorted(col_pts, self.mapped_buckets[ixs[1]]) + b[0]
                outlier_ixs.extend(self.sort_ixs[start:end])
                if stash_size > max_pts_to_stash:
                    break
            final_so, _ = scs.scan_overhead()
            final_overhead += final_so
        overhead_stats = {
                "final_overhead": final_overhead,
                "initial_overhead": initial_overhead,
                "total_true_points": total_true_pts
                }
        return np.array(outlier_ixs), overhead_stats

    def stash_all_outliers(self, max_pts_to_stash):
        stash_size = 0
        buckets_to_stash = []
        so = self.cg.scan_overhead()
        data = [(0, so)]
        while True:
            s, b, ixs = self.cg.pop_best()
            if s is None:
                break
            stash_size += s
            so = self.cg.scan_overhead()
            data.append((stash_size, so))
            for i in range(ixs[1], ixs[2]):
                buckets_to_stash.append(self.buckethash(ixs[0], i))
            if stash_size > max_pts_to_stash:
                break

        all_pts_hashes = self.buckethash(self.buckets[:,1], self.buckets[:, 0])
        outlier_ix_mask = np.isin(all_pts_hashes, buckets_to_stash)
        return np.where(outlier_ix_mask)[0], data
            
    def get_mapping(self):
        m = ccm.ContinuousCorrelationMap(self.mdim, self.tdim)
        grid = self.cg.grid
        # For each mapped column, get the ranges it corresponds to
        for j in range(grid.shape[1]):
            ranges = []
            in_range = False
            rstart, rend = 0, 0
            for i in range(grid.shape[0]):
                if grid[i,j] > 0:
                    if in_range:
                        rend = self.target_buckets[i+1]
                    else:
                        in_range = True
                        rstart = self.target_buckets[i]
                        rend = self.target_buckets[i+1]
                else:
                    if (in_range):
                        ranges.append((rstart, rend))
                    in_range = False
            # finish the last range
            if in_range:
                ranges.append((rstart, rend))
            m.add_mapping(self.mapped_buckets[j],
                          self.mapped_buckets[j+1],
                          ranges)
        return m


                

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

# Uses the single column stasher.
# Not an instance method because we don't want it to send around tons of data.
def stash_outliers_single(args):
    # pts are specified as indexes of map buckets, in sorted order.
    pts = args["points"]
    offset = args["offset"]
    kr = args["k"]
    max_pts_to_stash = args["stash_limit"]
    target_bix = args["target_bucket_ix"]
    alpha = args["alpha"]
    num_points = args["num_points"]
    tot_overhead = args["original_overhead"]
    stash_size = 0
    outlier_ixs = []
    if len(pts) == 0:
        return {
            "outlier_ixs": [],
            "final_overhead": 0,  # This computation is buggy 
            "initial_overhead": 0,
            "final_overhead_si": 0,
            "final_cost": 0,
            "total_true_pts": 0,
            "inlier_buckets": [],
            "outlier_iterations": []
            }
    # Make sure this is sorted
    assert np.all(np.diff(pts) >= 0)
    mb_bounds = [pts[0]-kr, pts[-1]+kr+1]
    map_buckets = np.arange(mb_bounds[0], mb_bounds[1], dtype=np.int32)
    counts = np.histogram(pts, bins=map_buckets)[0]
    counts_cpy = np.copy(counts)
    scs = SingleColumnStasher(counts, num_points, tot_overhead, kr, alpha=alpha)
    initial_overhead, true_pts = scs.scan_overhead()
   
    if max_pts_to_stash is None or max_pts_to_stash > 0:
        s, bucket_ixs = scs.pop_all_sort()
        stash_size = s
        starts = np.searchsorted(pts, bucket_ixs + mb_bounds[0]) + offset
        ends = np.searchsorted(pts, bucket_ixs + 1 + mb_bounds[0]) + offset
    #while max_pts_to_stash is None or stash_size < max_pts_to_stash:
    #    s, _, ixs = scs.pop_best()
    #    if s is None:
    #        break
    #    stash_size += s
    #    ixs1, ixs2 = ixs[0] + mb_bounds[0], ixs[1] + mb_bounds[0]
    #    start_ixs.append(ixs1)
    #    end_ixs.append(ixs2)
    #starts = np.searchsorted(pts, start_ixs) + offset
    #ends = np.searchsorted(pts, end_ixs) + offset
        for (s, e) in zip(starts, ends):
            outlier_ixs.extend(range(s, e))
    final_cost, final_so_si = 0, 0
    if tot_overhead > 0:
        final_cost, final_so_si = scs.cost()
    inlier_buckets = scs.inlier_buckets() + mb_bounds[0]

    return {
        "target_bucket_id": target_bix,
        "outlier_ixs": outlier_ixs,
        "inlier_buckets": inlier_buckets,
        "final_overhead": 0,  # This computation is buggy 
        "initial_overhead": initial_overhead,
        "final_overhead_si": final_so_si,
        "final_cost": final_cost,
        "total_true_pts": true_pts,
        "outlier_iterations": scs.get_outliers_by_iteration()
        }

class DummyBar(object):
    def __init__(self):
        pass
    def next(self):
        pass
    def finish(self):
        pass

class Stasher(object):
    # pts is a numpy array of shape n x 2. The 0th column is the mapped dimension, and the 1st
    # column is the target dimension.
    # k is the average length of the ranges on the mapped dimension.
    # buckets is an integer list of bucket ids for each point (size = (len(pts),))
    def __init__(self, data, mapped_schema, target_schemas, nproc=1, compute_overhead=True):
        self.using_display = True
        map_bucketer = Bucketer([mapped_schema], data, sequentialize=True)
        self.mapped_buckets = map_bucketer.get_ids()
        print("Got all mapped bucket IDs")
        target_bucketer = Bucketer(target_schemas, data)
        self.target_buckets = target_bucketer.get_ids()
        print("Got all target bucket IDs")
        # Make sure data is already sorted in order of the target bucket value.
        assert np.all(np.diff(self.target_buckets) >= 0)

        self.num_mapped_buckets = self.mapped_buckets.max()+1
        self.data = data
        self.outlier_indexes = []
        self.mapped_dim = map_bucketer.spec[0].dim
        self.target_dims = [s.dim for s in target_bucketer.spec]
        self.nproc = nproc
        
        self.ccm = ccm.ContinuousCorrelationMap()
        self.ccm.set_mapped_bucketer(map_bucketer)
        self.ccm.set_target_bucketer(target_bucketer)
        print("Done finding bucket IDs")
        
        self.bucket_ranges = self.bucketize()
        self.original_overhead = self.compute_overhead(nproc=nproc) if compute_overhead else 1
        self.bucketmap = defaultdict(list)
        # if True, will show progress bars

    def display(self, d):
        self.using_display = d

    def bucketize(self):
        # Bucket along the target dimension as the primary key. Secondary key is the mapped dimension
        # value.
        # Sort by bucket id then by mapped value
        mmax = self.mapped_buckets.max() + 1
        sortkey = self.target_buckets * mmax + self.mapped_buckets
        self.sort_ixs = np.argsort(sortkey)
        self.mapped_buckets = self.mapped_buckets[self.sort_ixs]
        self.target_buckets = self.target_buckets[self.sort_ixs]
        self.data = self.data[self.sort_ixs]
        uq, first, counts = np.unique(self.target_buckets, return_index=True, return_counts=True)
        print("# Mapped buckets (dim %d) = %d" % (self.mapped_dim,
            len(np.unique(self.mapped_buckets))))
        print("# Target buckets (dims %s) = %d" % (str(self.target_dims), len(uq)))
        self.target_bucket_counts = counts

        buckets = {}
        prev_end = 0
        for i, (u, f, c) in enumerate(zip(uq, first, counts)):
            assert c > 0
            assert f == prev_end
            buckets[u] = (f, f+c)
            prev_end = f+c
        return buckets

    def buckethash(self, row, col):
        return row * self.nbuckets_mapped + col

    def chunks(self, k, alpha, overhead, max_outliers_per_bucket=None):
        for bix, b in self.bucket_ranges.items():
            yield {
                "points": self.mapped_buckets[b[0]:b[1]],
                "offset": b[0],
                "k": k,
                "target_bucket_ix": bix,
                "stash_limit": max_outliers_per_bucket,
                "alpha": alpha,
                "num_points": len(self.mapped_buckets),
                "original_overhead": overhead 
                }
    
    def stash_outliers_parallel(self, kr, alpha, max_outliers_per_bucket=None):
        print("Finding outliers with nproc = %d, alpha=%f, k = %d, max outliers = %s" % \
                (self.nproc, alpha, kr, str(max_outliers_per_bucket)))
        cuml_outliers = []
        cuml_stats = defaultdict(float)

        # Use the parallelism to compute hte total overhead

        original_so = self.original_overhead
        print("Initial scan overhead:", original_so)

        # Map each mapped bucket to a list of target buckets that have inlier points.
        bucketmap = defaultdict(list)
        outlier_seq = None
        total_cost = 0

        with Pool(processes=self.nproc) as pool:
            inc = 10
            bar = DummyBar()
            if self.using_display:
                bar = ShadyBar("Finding outliers", max=len(self.bucket_ranges)/inc)
            i = 0
            cs = max(1, int(len(self.bucket_ranges)/self.nproc/50))
            for r in pool.imap_unordered(stash_outliers_single,
                    self.chunks(kr, alpha, original_so, max_outliers_per_bucket),
                    chunksize=cs):
                cuml_outliers.extend(np.unique(r['outlier_ixs']))
                cuml_stats["final_overhead"] += r["final_overhead"]
                cuml_stats["initial_overhead"] += r["initial_overhead"]
                cuml_stats["final_overhead_with_index"] += r["final_overhead_si"]
                cuml_stats["total_true_points"] += r["total_true_pts"]
                target_bix = r["target_bucket_id"]
                
                # TODO: do this at the end (in write_mapping) so we can use it for CM too
                for mb in r["inlier_buckets"]:
                    bucketmap[mb].append(target_bix)
                if outlier_seq is None:
                    outlier_seq = np.array(r["outlier_iterations"])
                else:
                    outlier_seq += np.array(r["outlier_iterations"])
                total_cost += r["final_cost"]
                i += 1
                if i % inc == 0:
                    bar.next()
            bar.finish()

        print("Outlier sequence:", outlier_seq)
        print("Total cost:", total_cost)
        
        cuml_stats["data_size"] = len(self.data)
        cuml_stats["total_cost"] = total_cost
        cuml_stats["num_outliers"] = len(cuml_outliers)
        self.outlier_indexes = cuml_outliers
        # ndices in the original dataset
        orig_outlier_list = self.sort_ixs[cuml_outliers]
        self.bucketmap = bucketmap
        self.ccm.set_bucket_map(bucketmap)
        return orig_outlier_list, cuml_stats

    # In the revised model, the host bucket isn't modified 
    def stash_outliers_fast(self, max_pts_to_stash):
        cuml_outliers = []
        cuml_stats = defaultdict(float)
        
        print("Total cost:", total_cost)
        
        cuml_stats["data_size"] = len(self.data)
        cuml_stats["total_cost"] = total_cost
        cuml_stats["num_outliers"] = len(cuml_outliers)
        self.outlier_indexes = cuml_outliers
        # ndices in the original dataset
        orig_outlier_list = self.sort_ixs[cuml_outliers]
        self.ccm.set_bucket_map(bucketmap)
        return orig_outlier_list, cuml_stats

    def compute_overhead(self, nproc=1):
        original_so = 0
        with Pool(processes=nproc) as pool:
            inc = 10
            bar = DummyBar()
            if self.using_display:
                bar = ShadyBar("Computing overhead", max=len(self.bucket_ranges)/inc)
            i = 0
            cs = max(1, int(len(self.bucket_ranges)/nproc/50))
            for r in pool.imap_unordered(stash_outliers_single, self.chunks(1, 0, 0), chunksize=cs):
                original_so += r["initial_overhead"]
                i += 1
                if i % inc == 0:
                    bar.next()
            bar.finish()
        return original_so

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

        ntarget_buckets = self.target_buckets.max() + 1

        # Dump information for the indexer
        outlier_list_file = prefix + ".outliers"
        target_bucket_file = prefix + ".targets"
        
        # These are the outlier indexes in the original dataset
        self.sort_ixs[self.outlier_indexes].astype(int).tofile(outlier_list_file)
        
        # Write the mapping from target bucket to physical index range in the sorted data 
        fobj = open(target_bucket_file, 'w')
        fobj.write("target_index_ranges\t%d\n" % len(self.bucket_ranges))
        for ub, (f, e) in self.bucket_ranges.items():
            if ub == OUTLIER_BUCKET:
                continue
            fobj.write("%d\t%d\t%d\n" % (ub, f, e))
        

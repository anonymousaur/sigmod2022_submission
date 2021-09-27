import numpy as np
import time

np.random.seed(int(time.time()))

INIT_STASH_P = 0.025 #np.random.uniform()

# Like convolution grid, but on a single row along the mapped dimension, for a single bucket along
# the target dimension (or combination of target dimensions).
# The burden is on the caller to define what the buckets are, and send this object only the points
# within a single bucket
class SingleColumnStasher(object):
    # Counts is the number of points per bucket in the mapped dimension
    # k is the number of buckets the average query spans
    # alpha is the relative importance of the number of outliers compared to the scan overhead
    # saved. A higher value of alpha will prioritize keeping the outlier number low (i.e. reducing
    # the size of storage for the outliers), and a lower value will prioritize minimizing scan
    # overhead.
    def __init__(self, counts, num_points, total_overhead, k, alpha=1):
        self.counts = counts
        self.npoints = num_points
        self.orig_overhead = total_overhead
        self.alpha = alpha
        self.original_counts = np.copy(counts)
        self.k = k
        assert len(counts) >= k, "k > grid size: %d vs %d" % (k, len(counts))
        self.arange_reset = np.arange(k)
        self.cumul_count = np.sum(counts)
        self.init_zeros()
        self.init_benefit()
        # For each bucket id in the target dimension, stores the total number of points removed
        # among all buckets with that bucket id. 
        self.removed = np.zeros(len(counts), dtype=int)
        self.outliers_by_iteration = []

    def init_zeros(self):
        self.zeros_left = np.zeros(len(self.counts), dtype=np.int32)
        self.zeros_right = np.zeros(len(self.counts), dtype=np.int32)
        rowsize = len(self.counts)
        z_left, z_right = -1, rowsize 
        for j in range(rowsize):
            if self.counts[j] == 0:
                self.zeros_left[j] = j-1
            else:
                self.zeros_left[j] = max(z_left, j-self.k)
                z_left = j
        for j in range(rowsize):
            if self.counts[rowsize-j-1] == 0:
                self.zeros_right[rowsize-j-1] = rowsize-j
            else:
                self.zeros_right[rowsize-j-1] = min(z_right, rowsize-j-1+self.k)
                z_right = rowsize-j-1

    def update_zeros(self, ix, k):
        self.zeros_left[ix:ix+k] = self.arange_reset + ix - 1 
        self.zeros_right[ix:ix+k] = self.arange_reset + ix + 1
        rowsize = len(self.counts)
        for i in range(ix + k, min(rowsize, ix + 2*k - 1)):
            if self.zeros_left[i] < ix + k and self.counts[i] > 0:
                self.zeros_left[i] = i - k
        for i in range(max(0, ix - k + 1), ix):
            if self.zeros_right[i] >= ix and self.counts[i] > 0:
                self.zeros_right[i] = i + k

    def init_benefit(self):
        self.benefit = np.zeros(len(self.counts)-self.k+1, dtype=int)
        self.benefit2 = np.zeros(len(self.counts)-self.k+1, dtype=np.float64)
        self.update_benefits(0, len(self.benefit))
        
    def update_benefits(self, start, end):
        # Computes the "benefit" of removing the k elements beginning at each position. The benefit
        # is the number of points we avoid scanning by removing the queries minus the number of
        # extra points we have to scan in the outlier buffer.
        tot_nq = len(self.benefit)
        for i in range(start, end):
            # Number of queries eliminated depends on how many zeros are to the left and right of
            # the edgemost non-zero elements in each k-block.
            # TODO: we can compute this a bit faster using zeros_left and zeros_right to find the
            # edgemost non-zero elements in the k-block. No need for a linear time scan.
            npts = np.sum(self.counts[i:i+self.k])
            if npts == 0:
                self.benefit[i] = 0
                self.benefit2[i] = 0
            else:
                z_left = np.min(self.zeros_left[i:i+self.k])
                z_right = np.max(self.zeros_right[i:i+self.k])
                queries_elim = z_right - z_left - self.k
                self.benefit[i] = queries_elim*(self.cumul_count - npts) - self.alpha*tot_nq*npts
                self.benefit2[i] = float(queries_elim) / npts
                if self.cumul_count < float(npts * tot_nq) / queries_elim:
                    self.benefit2[i] = 0

    def pop_best(self):
        col_ix = self.benefit.argmax()
        ben = self.benefit[col_ix]
        if ben <= 0:
            return None, None, None
        stash_size = self.counts[col_ix:col_ix+self.k].sum()
        self.cumul_count -= stash_size
        self.removed[col_ix:col_ix+self.k] += self.counts[col_ix:col_ix+self.k]
        self.counts[col_ix:col_ix+self.k] = 0
        self.update_zeros(col_ix, self.k)
        # TODO: make this faster by storing the number of eliminated queries, so we can do a one
        # time subtraction using cumul_count, which is the only non-local thing that gets updated.
        self.update_benefits(0, len(self.benefit))
                #max(0, col_ix-self.k+1),
                #min(col_ix+2*self.k-1, self.benefit.shape[1])) 
        return stash_size, ben, (col_ix, col_ix+self.k)

    # Remove all points that have a postiive benefit. Like the original method, this isn't optimal.
    def pop_all(self):
        niters = 10
        combined_counts = np.zeros(len(self.counts))
        z_lefts = np.zeros_like(self.zeros_left) + len(self.counts)
        z_rights = np.zeros_like(self.zeros_right)
        for i in range(self.k):
            combined_counts += np.roll(self.counts, -i)
            z_lefts = np.minimum(z_lefts, np.roll(self.zeros_left, -i))
            z_rights = np.maximum(z_rights, np.roll(self.zeros_right, -i))
        q_elim = z_rights - z_lefts - self.k
        factor = self.orig_overhead / self.npoints
        
        buckets_to_stash = None
        # Start by assuming nothing else is stashed.
        leftover = self.counts.sum() - self.counts
        for nit in range(niters):
            benefits = q_elim*leftover - self.alpha*factor*combined_counts
            # These are the indices with the overflow from np.roll. Remove them.
            ixs_to_stash = benefits > 0
            ixs_to_stash[len(benefits)-self.k+1:] = False
            # If k > 1, there may be overlapping buckets here. Dedup
            buckets_to_stash = np.zeros_like(ixs_to_stash)
            for i in range(self.k):
                buckets_to_stash = np.logical_or(buckets_to_stash, np.roll(ixs_to_stash, i))
            stashed = self.counts[buckets_to_stash].sum()
            self.outliers_by_iteration.append(stashed)
            # This is an expensive computation. Do it only when we need to.
            if nit < niters-1:
                # unroll to figure out which combined buckets are stashed
                # Figure out how many of each combined bucket has been left *unstashed*.
                unstashed_pts = self.counts * np.logical_not(buckets_to_stash)
                comb_buckets_unstashed = np.zeros(len(benefits), int)
                for i in range(self.k):
                    comb_buckets_unstashed += np.roll(unstashed_pts, -i)
                comb_buckets_unstashed[len(comb_buckets_unstashed)-self.k+1:] = 0
                leftover = unstashed_pts.sum() - comb_buckets_unstashed

        self.removed[buckets_to_stash] = self.counts[buckets_to_stash]
        bucket_ixs_stashed = np.where(buckets_to_stash)[0]
        self.counts[buckets_to_stash] = 0
        return self.removed.sum(), bucket_ixs_stashed

    def pop_all_test_convergence(self):
        niters = 0
        combined_counts = np.zeros(len(self.counts))
        z_lefts = np.zeros_like(self.zeros_left) + len(self.counts)
        z_rights = np.zeros_like(self.zeros_right)
        for i in range(self.k):
            combined_counts += np.roll(self.counts, -i)
            z_lefts = np.minimum(z_lefts, np.roll(self.zeros_left, -i))
            z_rights = np.maximum(z_rights, np.roll(self.zeros_right, -i))
        q_elim = z_rights - z_lefts - self.k
        cumul_cnt = self.counts.sum()
        factor = self.orig_overhead / self.npoints
        
        p = INIT_STASH_P
        buckets_to_stash = np.random.choice([0,1], len(self.counts), replace=True, p=[1-p, p]).astype(bool)
        unstashed_pts = self.counts*np.logical_not(buckets_to_stash)
        # remove each unstashed bucket from the total for the leftover.
        leftover = unstashed_pts.sum() - unstashed_pts
        self.outliers_by_iteration.append(self.counts[buckets_to_stash].sum())

        for nit in range(niters):
            benefits = q_elim*leftover - self.alpha*factor*combined_counts
            # These are the indices with the overflow from np.roll. Remove them.
            ixs_to_stash = benefits > 0
            ixs_to_stash[len(benefits)-self.k+1:] = False
            # If k > 1, there may be overlapping buckets here. Dedup
            buckets_to_stash = np.zeros_like(ixs_to_stash)
            for i in range(self.k):
                buckets_to_stash = np.logical_or(buckets_to_stash, np.roll(ixs_to_stash, i))
            stashed = self.counts[buckets_to_stash].sum()
            self.outliers_by_iteration.append(stashed)
            # This is an expensive computation. Do it only when we need to.
            if nit < niters-1:
                # unroll to figure out which combined buckets are stashed
                # Figure out how many of each combined bucket has been left *unstashed*.
                unstashed_pts = self.counts * np.logical_not(buckets_to_stash)
                comb_buckets_unstashed = np.zeros(len(benefits), int)
                for i in range(self.k):
                    comb_buckets_unstashed += np.roll(unstashed_pts, -i)
                comb_buckets_unstashed[len(comb_buckets_unstashed)-self.k+1:] = 0
                leftover = unstashed_pts.sum() - comb_buckets_unstashed

        self.removed[buckets_to_stash] = self.counts[buckets_to_stash]
        bucket_ixs_stashed = np.where(buckets_to_stash)[0]
        self.counts[buckets_to_stash] = 0
        return self.removed.sum(), bucket_ixs_stashed

    def pop_all_sort(self):
        assert self.k == 1
        combined_counts = np.zeros(len(self.counts))
        z_lefts = np.zeros_like(self.zeros_left) + len(self.counts)
        z_rights = np.zeros_like(self.zeros_right)
        for i in range(self.k):
            combined_counts += np.roll(self.counts, -i)
            z_lefts = np.minimum(z_lefts, np.roll(self.zeros_left, -i))
            z_rights = np.maximum(z_rights, np.roll(self.zeros_right, -i))
        q_elim = z_rights - z_lefts - self.k
        cumul_cnt = self.counts.sum()
        true_pts = self.npoints
        factor = (self.orig_overhead + true_pts) / self.npoints 
        # How much more an outlier adds to performance than a point in a range.
        outlier_performance_factor = 5

        # for each bucket, the minimum number of leftover points that must be unstashed for stashing that
        # bucket to be worth it. If there are more leftover, it's only more of a reason to stash
        # that bucket.
        sort_ix = np.argsort(combined_counts)
        # Go through each bucket in order and stash it, until the min_leftover[i] exceeds the
        # amount stashed so far
        stashed = 0
        bucket_ixs_stashed = None
        min_bucket = len(combined_counts)
        for i, c in enumerate(combined_counts[sort_ix]):
            if c == 0:
                continue
            min_bucket = min(min_bucket, i)
            # By stashing this bucket, the marginal overhead is reduced.
            marginal_overhead_red = cumul_cnt - stashed - c
            # All the unstashed buckets don't have to count this bucket as extra overhead
            marginal_overhead_red += (len(self.counts) - i - 1)*c
            # However, the marginal storage factor and query time increase is positive.
            marginal_storage_gain = c * (outlier_performance_factor - 1 + self.alpha*factor)
            if marginal_overhead_red <= marginal_storage_gain:
                bucket_ixs_stashed = sort_ix[min_bucket:i]
                break
            stashed += c
        if bucket_ixs_stashed is None:
            bucket_ixs_stashed = range(min_bucket, len(combined_counts))
        
        self.removed[bucket_ixs_stashed] = self.counts[bucket_ixs_stashed]
        self.counts[bucket_ixs_stashed] = 0
        return stashed, bucket_ixs_stashed

    # Returns the number of extra points scanned.
    # TODO(vikram): Only secondary_index=True works here.
    def scan_overhead(self, secondary_index=True):
        overhead = 0
        total_true_pts = 0
        total_unstashed = self.counts.sum()
        assert self.removed.sum() + total_unstashed == self.original_counts.sum()
        for i in range(len(self.benefit)):
            true_pts = self.original_counts[i:i+self.k].sum()
            total_true_pts += true_pts
            is_accessed = self.counts[i:i+self.k].sum() > 0
            accessed = is_accessed * total_unstashed 
            if secondary_index:
                overhead += self.removed[i:i+self.k].sum() + accessed - true_pts
            else:
                overhead += self.removed.sum() + accessed - true_pts
        return overhead, total_true_pts

    def cost(self):
        s = self.scan_overhead()[0]
        cost_model = s/self.orig_overhead + self.alpha*self.removed.sum()/self.npoints
        return cost_model, s

    # At this point in the computation, get a boolean array with True if the corresponding index in
    # the original `counts` array has points that haven't been stashed.
    def inlier_buckets(self):
        return np.where(self.counts > 0)[0]

    def outlier_buckets(self):
        return np.where(self.original_counts != self.counts)[0]

    def get_outliers_by_iteration(self):
        return self.outliers_by_iteration

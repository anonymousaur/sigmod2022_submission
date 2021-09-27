import numpy as np

# Keeps track of the points that need pruning.
class GridStasher(object):
    # Grid is a 2D array of integers denoting how many points are in each bucket
    # k is the size of the average query (how many buckets it spans)
    def __init__(self, grid, k):
        # Axis 0 of grid is the target dimension, axis 1 is the mapped dimension
        self.grid = np.array(grid)
        self.original_grid_col_cumul = np.sum(self.grid, axis=0)
        if len(self.grid.shape) < 2:
            self.grid = self.grid.reshape(-1, len(grid))
        self.k = k
        self.arange_reset = np.arange(k)
        self.grid_cumul = self.grid.sum(axis=1)
        self.queries_removed = np.zeros(grid.shape[0], dtype=int)
        self.init_zeros()
        self.init_benefit()
        mask = (grid == 0)
        # This is a masked array, so we can better deal with invalid values
        self.maxs = np.zeros(grid.shape[0])
        self.argmaxs = np.zeros(grid.shape[0], dtype=int)
        for i in range(grid.shape[0]):
            self.update_maxs(i)
        self.removed_per_row = np.zeros(grid.shape[0])

    def init_zeros(self):
        # The zeros track the index of the first non-zero element to the left / right of each
        # position, up to a max of k, but only for the indexes corresponding to non-zero elements in
        # self.grid. For all elements in self.grid that are 0, the value of zeros_left[i][j] = j-1,
        # i.e. the index immediately to the left (likewise for zeros_right)
        self.zeros_right = np.zeros(self.grid.shape, dtype=np.int16)
        self.zeros_left = np.zeros(self.grid.shape, dtype=np.int16)
        rowsize = self.grid.shape[1]
        for i in range(self.grid.shape[0]):
            z_left, z_right = -1, rowsize 
            for j in range(rowsize):
                if self.grid[i][j] == 0:
                    self.zeros_left[i][j] = j-1
                else:
                    self.zeros_left[i][j] = max(z_left, j-self.k)
                    z_left = j
            for j in range(rowsize):
                if self.grid[i][rowsize-j-1] == 0:
                    self.zeros_right[i][rowsize-j-1] = rowsize-j
                else:
                    self.zeros_right[i][rowsize-j-1] = min(z_right, rowsize-j-1+self.k)
                    z_right = rowsize-j-1

    # Updates the zeros_* arrays given that the elements at [row, col_start:col_end] are now set to
    # 0.
    def update_zeros(self, row_ix, col_ix, k):
        self.zeros_left[row_ix, col_ix:col_ix+k] = self.arange_reset + col_ix - 1 
        self.zeros_right[row_ix, col_ix:col_ix+k] = self.arange_reset + col_ix + 1
        rowsize = self.zeros_left.shape[1]
        for i in range(col_ix + k, min(rowsize, col_ix + 2*k - 1)):
            if self.zeros_left[row_ix, i] < col_ix + k and self.grid[row_ix, i] > 0:
                self.zeros_left[row_ix, i] = i - k
        for i in range(max(0, col_ix - k + 1), col_ix):
            if self.zeros_right[row_ix, i] >= col_ix and self.grid[row_ix, i] > 0:
                self.zeros_right[row_ix, i] = i + k


    def init_benefit(self):
        self.benefit = np.zeros((self.grid.shape[0], self.grid.shape[1]-self.k+1), dtype=int)
        self.benefit2 = np.zeros((self.grid.shape[0], self.grid.shape[1]-self.k+1),
                dtype=np.float64)
        for i in range(self.grid.shape[0]):
            self.update_benefits(i, 0, self.benefit.shape[1])


    def update_benefits(self, row, col_start, col_end):
        # Computes the "benefit" of removing the k elements beginning at each position. The benefit
        # is the number of points we avoid scanning by removing the queries minus the number of
        # extra points we have to scan in the outlier buffer.
        tot_nq = self.benefit.shape[1]
        for i in range(col_start, col_end):
            # Number of queries eliminated depends on how many zeros are to the left and right of
            # the edgemost non-zero elements in each k-block.
            # TODO: we can compute this a bit faster using zeros_left and zeros_right to find the
            # edgemost non-zero elements in the k-block. No need for a linear time scan.
            npts = np.sum(self.grid[row, i:i+self.k])
            if npts == 0:
                self.benefit[row, i] = 0
                self.benefit2[row, i] = 0
            else:
                z_left = np.min(self.zeros_left[row, i:i+self.k])
                z_right = np.max(self.zeros_right[row, i:i+self.k])
                queries_elim = z_right - z_left - self.k
                #self.benefit[row, i] = queries_elim*(self.grid_cumul[row]-npts) - tot_nq*npts \
                #        - npts * self.queries_removed[row]
                self.benefit[row, i] = queries_elim*self.grid_cumul[row] - tot_nq*npts
                self.queries_removed[row] += queries_elim 
                self.benefit2[row, i] = float(queries_elim) / npts
                if self.grid_cumul[row] < float(npts * tot_nq) / queries_elim:
                    self.benefit2[row, i] = 0


    def update_maxs(self, row_ix):
        # Compute number of total points in that row minus the points matching the average query.
        col = self.benefit[row_ix].argmax()
        self.argmaxs[row_ix] = col
        self.maxs[row_ix] = self.benefit[row_ix, col]
        

    def pop_best(self):
        row_ix = self.maxs.argmax()
        col_ix = self.argmaxs[row_ix]
        ben = self.benefit[row_ix, col_ix]
        if ben <= 0:
            return None, None, None
        stash_size = self.grid[row_ix, col_ix:col_ix+self.k].sum()
        self.grid_cumul[row_ix] -= stash_size
        self.grid[row_ix, col_ix:col_ix+self.k] = 0
        self.update_zeros(row_ix, col_ix, self.k)
        # TODO: make this faster by storing the number of eliminated queries, so we can do a one
        # time subtraction using grid_cumul, which is the only non-local thing that gets updated.
        self.update_benefits(row_ix, 0, self.benefit.shape[1])
                #max(0, col_ix-self.k+1),
                #min(col_ix+2*self.k-1, self.benefit.shape[1])) 
        self.update_maxs(row_ix)
        self.removed_per_row[row_ix] += stash_size
        return stash_size, ben, (row_ix, col_ix, col_ix+self.k)

    def scan_overhead(self):
        overhead = 0
        for i in range(self.benefit.shape[1]):
            true_pts = self.original_grid_col_cumul[i:i+self.k].sum()
            rows_accessed = self.grid[:, i:i+self.k].sum(axis=1) > 0
            accessed_per_row = rows_accessed * self.grid_cumul
            overhead += self.removed_per_row.sum() + accessed_per_row.sum() - true_pts
        return overhead

    def grid(self):
        return np.copy(self.grid)


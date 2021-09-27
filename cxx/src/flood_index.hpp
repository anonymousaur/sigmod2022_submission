#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cassert>
#include <set>
#include <vector>
#include <limits>

#include "flood_index.h"

// index_dir should contain index_file.bin, as well as files for
// functional mapping models/errors and non-uniform grid secondary
// boundaries.
// Everything in index_file is an int64_t
// File format (no line breaks in the actual file):
// [num indexed dims] [[indexed dims]]  (final indexed dim is sort dim)
// [num grid dims] [[grid dims in order]] [[num cols per grid dim in order]]
// [num functional mappings] [[ (mapped dim, target dim) ]]
// [num nonuniform grids] [[ (base dim, secondary dim) ]]
template <size_t D>
FloodIndex<D>::FloodIndex(const std::string& index_dir)
    : indexed_dims_(), grid_dims_order_(), grid_dims_() {
  dataset_ = nullptr;

  std::string index_file = index_dir + "/index_file.bin";
  std::cout << "Loading " << index_file << std::endl;
  std::vector<int64_t> content = load_binary_file<int64_t>(index_file);

  std::cout << "File content: ";
  for (int64_t p : content) {
    std::cout << p << " ";
  }
  std::cout << std::endl;

  int cix = 0;  // cursor index

  // [num indexed dims] [[indexed dims]]  (final indexed dim is sort dim)
  int num_indexed_dims = static_cast<int>(content[cix++]);
  indexed_dims_.resize(num_indexed_dims);
  for (int i = 0; i < num_indexed_dims; i++) {
    indexed_dims_[i] = static_cast<int>(content[cix++]);
  }
  sort_dim_ = indexed_dims_.back();
  if (sort_dim_ < 0) {
    indexed_dims_.pop_back();
  }

  // [num grid dims] [[grid dims in order]] [[num cols per grid dim in order]]
  n_grid_dims_ = static_cast<int>(content[cix++]);
  std::vector<int> all_dims_order(n_grid_dims_);
  for (int i = 0; i < n_grid_dims_; i++) {
    all_dims_order[i] = static_cast<int>(content[cix++]);
  }
  std::vector<int> all_cols(n_grid_dims_);
  std::vector<int> nontrivials;
  for (int i = 0; i < n_grid_dims_; i++) {
    all_cols[i] = static_cast<int>(content[cix++]);
    if (all_cols[i] > 1) {
        nontrivials.push_back(i);
    }
  }
  std::vector<int> n_cols;
  n_grid_dims_ = nontrivials.size();
  for (int ix : nontrivials) {
    grid_dims_order_.push_back(all_dims_order[ix]);
    n_cols.push_back(all_cols[ix]);
  }
   
  std::vector<int> multipliers(n_grid_dims_, 1);
  for (int i = n_grid_dims_ - 2; i >= 0; i--) {
    multipliers[i] = multipliers[i+1] * n_cols[i+1];
  }
  for (int i = 0; i < static_cast<int>(grid_dims_order_.size()); i++) {
    int dim = grid_dims_order_[i];
    grid_dims_[dim] = { dim, i, n_cols[i], multipliers[i] };
    std::cout << "Grid dim " << i << ": " << dim << " with " << n_cols[i] << " columns" << std::endl;
  }

  // [num functional mappings] [[ (mapped dim, target dim) ]]
  int n_functional_mappings = static_cast<int>(content[cix++]);
  assert (n_functional_mappings == 0);
  
  // [num nonuniform grids] [[ (base dim, secondary dim) ]]
  int n_nonuniform_grids = static_cast<int>(content[cix++]);
  assert (n_nonuniform_grids == 0);
  n_base_dims_ = 0;
  n_uniform_dims_ = n_grid_dims_ - n_nonuniform_grids;

#ifndef NDEBUG
  // Check correctness
  // Base dims come before other dims
  for (int base_dim : base_dims) {
    int gix = std::find(grid_dims_order_.begin(), grid_dims_order_.end(), base_dim) - grid_dims_order_.begin();
    assert(gix < n_base_dims_);
  }
  // Uniform dims come before nonuniform dims
  for (const auto& x : nonuniform_grids_) {
    int gix = std::find(grid_dims_order_.begin(), grid_dims_order_.end(), x.first) - grid_dims_order_.begin();
    assert(gix >= n_uniform_dims_);
  }
  // Target dims must be uniform dims
  for (const auto& x : functional_mappings_) {
    int gix = std::find(grid_dims_order_.begin(), grid_dims_order_.end(), x.second.target_dim) - grid_dims_order_.begin();
    assert(gix < n_uniform_dims_);
  }
#endif

  // Load partition boundaries for each grid dimension
  for (const auto& x : grid_dims_) {
    int dim = x.first;
    int ncols = x.second.n_cols;
    std::string boundaries_file = index_dir + "/boundaries_" + std::to_string(dim) + "_" + std::to_string(ncols) + ".bin";
    std::cout << "Loading boundary file " << boundaries_file << std::endl;
    partition_boundaries_[dim] = load_binary_file<Scalar>(boundaries_file);
    std::cout << "Done." << std::endl;
  }
}

template <size_t D>
void FloodIndex<D>::SetDataset(std::shared_ptr<Dataset<D>> dataset) {
  dataset_ = dataset;
}

// This also initializes the cell boundaries structure.
// Stores the start offset because that denotes the part of the dataset that this index is
// responsible for.
template <size_t D>
bool FloodIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    int64_t n_points = std::distance(start, end);
    int64_t start_offset = 0;

    // Find mins and maxs
    for (size_t i = 0; i < D; i++) {
      dim_mins_[i] = std::numeric_limits<Scalar>::max();
      dim_maxs_[i] = std::numeric_limits<Scalar>::lowest();
    }
    for (auto it = start; it != end; it++) {
      const Point<D>& p = *it;
      for (size_t i = 0; i < D; i++) {
        dim_mins_[i] = std::fmin(p[i], dim_mins_[i]);
        dim_maxs_[i] = std::fmax(p[i], dim_maxs_[i]);
      }
    }

    // Sort data
    std::vector<std::pair<AGVirtualIndex, int>> indices(n_points);
    if (sort_dim_ < 0) {
      int i = 0;
      for (auto it = start; it != end; it++, i++) {
        indices[i] = std::make_pair(AGVirtualIndex{get_cell_number(*it), 0}, i);
      }
    } else {
      int i = 0;
      for (auto it = start; it != end; it++, i++) {
          indices[i] = std::make_pair(AGVirtualIndex{get_cell_number(*it), (*it)[sort_dim_]}, i);
      }
    }
    std::stable_sort(indices.begin(), indices.end(),
              [](const std::pair<AGVirtualIndex, int>& a, const std::pair<AGVirtualIndex, size_t>& b) -> bool {
                return a.first.cell_no < b.first.cell_no || (a.first.cell_no == b.first.cell_no && a.first.sort_val < b.first.sort_val);
              });
    
    std::vector<Point<D>> sorted_data(n_points);
    std::transform(indices.begin(), indices.end(), sorted_data.begin(),
              [start](const auto& ix_pair) -> Point<D> {
                  return *(start + ix_pair.second);
                  });
    // Write to stdout, so we can run Cortex on it
    int prev_cell_num = std::numeric_limits<int>::lowest();
    bool modified = false;
    std::ofstream sorted_data_buckets_("flood_sorted_buckets.dat");
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i].first.cell_no != prev_cell_num) {
            if (prev_cell_num >= 0) {
                sorted_data_buckets_ << i << std::endl;
            }
            sorted_data_buckets_ << i << ", ";
            prev_cell_num = indices[i].first.cell_no;
        }
        modified |= (indices[i].second != i);
    }
    std::cout << "Data was " << (modified ? "" : "not") << " modified" << std::endl;

    sorted_data_buckets_ << indices.size() << std::endl;
    sorted_data_buckets_.flush();
    sorted_data_buckets_.close();

    std::ofstream sorted_data_points_("flood_sorted_points.bin");
    size_t write_size = sizeof(Point<D>) * sorted_data.size();
    sorted_data_points_.write((char *)&*(sorted_data.begin()), write_size);
    sorted_data_points_.flush();
    sorted_data_points_.close();
    
    std::copy(sorted_data.begin(), sorted_data.end(), start);

    // Find cell boundaries.
    // Uses the fact that indices is sorted.
    int n_cells = num_cells();
    cell_boundaries_.reserve(n_cells+1);
    cell_boundaries_.push_back(start_offset);
    int n_cells_with_data = 0;
    int64_t idx = 0;
    int cell_start_idx = 0;
    int cur_cell = 0;
    while (idx < n_points) {
      while (idx < n_points && indices[idx].first.cell_no == cur_cell) {
        idx++;
      }
      int n_points_in_cell = idx - cell_start_idx;
      n_cells_with_data += static_cast<int>(n_points_in_cell > 0);
      cell_start_idx = idx;
      int next_cell;
      if (idx == n_points) {
        next_cell = n_cells;
      } else {
        next_cell = indices[idx].first.cell_no;
      }
      for (int i = cur_cell; i < next_cell; i++) {
        cell_boundaries_.push_back(static_cast<PhysicalIndex>(idx) + start_offset);
      }
      cur_cell = next_cell;
    }
    std::cout << "Boundaries: " << n_cells_with_data << " of " << n_cells << " cells have data" << std::endl;
    return modified;
}

/*template <size_t D>
void FloodIndex<D>::query(const Query<D> &query, Visitor<D> &visitor) {
  // Does not include sort dim, because the range over the sort dim will
  // be exact after refinement.
  std::vector<int> query_dimensions;
  bool should_search = false;
  for (int i = 0; i < static_cast<int>(D); i++) {
    if (!full_intersection(query.start[i], query.end[i], i)) {
      if (i == sort_dim_) {
        should_search = true;
      } else {
        query_dimensions.push_back(i);
      }
    }
    if (no_intersection(query.start[i], query.end[i], i)) {
      // Query does not intersect data at all.
      return;
    }
  }

  // Project
#if STATS
  auto start_time = std::chrono::high_resolution_clock::now();
#endif
  // Range ends should be exclusive
  std::vector<FloodIndex<D>::AGRange> ranges = get_query_ranges(query);
#if STATS
  auto finish_time = std::chrono::high_resolution_clock::now();
  auto projector_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
  LOG("projector_time", projector_time);
#endif

  // Rectify
#if STATS
  double avg_log_search_range = 0;
//  double log_avg_search_range = 0;
  start_time = std::chrono::high_resolution_clock::now();
#endif
  if (should_search) {
    Scalar sort_dim_min = query.start[sort_dim_];
    Scalar sort_dim_max = query.end[sort_dim_];
    for (FloodIndex<D>::AGRange& range : ranges) {
#if STATS
      avg_log_search_range += std::log2(range.end_ix - range.start_ix);
//      log_avg_search_range += range.end_ix - range.start_ix;
#endif
      range.start_ix = binary_search_lower_bound(range.start_ix, range.end_ix, sort_dim_min);
      range.end_ix = binary_search_upper_bound(range.start_ix, range.end_ix, sort_dim_max);
    }
#if STATS
    avg_log_search_range /= static_cast<double>(ranges.size());
//    log_avg_search_range = std::log2(log_avg_search_range / static_cast<double>(ranges.size()));
#endif
  }
#if STATS
  finish_time = std::chrono::high_resolution_clock::now();
  auto rectifier_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
  LOG("rectifier_time", rectifier_time);
  LOG("avg_log_search_range", avg_log_search_range);
//  LOG("log_avg_search_range", log_avg_search_range);
#endif

  // Scan
#if STATS
  size_t visited_points = 0;
  size_t valid_points = 0;
  size_t exact_visited_points = 0;
  int num_scanned_ranges = 0;
  int num_scanned_exact_ranges = 0;
  start_time = std::chrono::high_resolution_clock::now();
#endif
  for (const FloodIndex<D>::AGRange& range : ranges) {
    if (range.end_ix <= range.start_ix) {
      continue;
    }
#if STATS
    visited_points += range.end_ix - range.start_ix;
    num_scanned_ranges++;
#endif
    if (range.exact) {
#if STATS
      num_scanned_exact_ranges++;
      valid_points += range.end_ix - range.start_ix;
      exact_visited_points += range.end_ix - range.start_ix;
#endif
      visitor.visitExactRange(dataset_, range.start_ix, range.end_ix);
    } else {
      for (PhysicalIndex p = range.start_ix; p < range.end_ix; p += 64) {
        PhysicalIndex true_end = std::min(range.end_ix, p + 64);
        // A way to get the last true_end - p bits set to 1.
        uint64_t valids = 1ULL + (((1ULL << (true_end - p - 1)) - 1ULL) << 1);
        for (int dim : query_dimensions) {
          valids &= dataset_->GetCoordRange(p, true_end, dim, query.start[dim], query.end[dim]);
        }
#if STATS
        valid_points += __builtin_popcountll(valids);
#endif
        visitor.visitRange(dataset_, p, true_end, valids);
      }
    }
  }
#if STATS
  finish_time = std::chrono::high_resolution_clock::now();
  auto scan_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
  LOG("scan_time", scan_time);

  LOG("exact_visited_points", exact_visited_points);
  LOG("visited_points", visited_points);
  LOG("valid_points", valid_points);
  LOG("num_ranges", ranges.size());
  LOG("num_searched_ranges", should_search ? ranges.size() : 0 );
  LOG("num_scanned_ranges", num_scanned_ranges);
  LOG("num_scanned_exact_ranges", num_scanned_exact_ranges);
  LOG("single_scans", visited_points * query_dimensions.size());
  LOG("exact_single_scans", exact_visited_points * query_dimensions.size());
#endif
}*/

// Returns completed filled-out range structs.
// Only returns non-empty query ranges.
template <size_t D>
Set<PhysicalIndex> FloodIndex<D>::IndexRanges(Query<D>& query) {
  int num_query_dims = 0;
  int num_query_dims_in_index = 0;
  // Treat trailing dims that aren't selected in the query as effectively
  // not in the grid. This allows us to process many cells as
  // a batch, instead of one at a time.
  int effective_grid_depth = 0;
  for (int i = 0; i < static_cast<int>(D); i++) {
    if (query.filters[i].present) {
      num_query_dims++;
      auto loc = std::find(grid_dims_order_.begin(), grid_dims_order_.end(), i);
      if (loc != grid_dims_order_.end()) {
        num_query_dims_in_index++;
        effective_grid_depth = std::max(effective_grid_depth, (int)(loc - grid_dims_order_.begin()))+1;
      }
    }
  }
  // If none of the queried dims are contained in the indexed dims, do a full scan on everything.
  bool no_containment = (num_query_dims_in_index == 0);
  if (no_containment) {
      Ranges<PhysicalIndex> r {{0, cell_boundaries_.back()}};
      Set<PhysicalIndex> full(r, List<PhysicalIndex>());
      return full;
  }

  // Translate Cortex queries to Flood queries.
  Point<D> q_start, q_end;
  for (int i = 0; i < D; i++) {
    if (!query.filters[i].present) {
        q_start[i] = SCALAR_NINF;
        q_end[i] = SCALAR_PINF;
    } else {
        if (query.filters[i].is_range) {
            q_start[i] = query.filters[i].ranges[0].first;
            q_end[i] = query.filters[i].ranges[0].second;
        } else {
            q_start[i] = query.filters[i].values[0];
            q_end[i] = query.filters[i].values.back();
        }
    }
  }

  // Initialize the range of columns in each *uniform* grid dimension.
  // Non-uniform grid dimensions will be processed per hypercell.
  // Stored in order of grid dimensions, not order of original dataset dims.
  std::vector<int> cur_cols(effective_grid_depth);
  std::vector<int> start_cols(effective_grid_depth);
  std::vector<int> end_cols(effective_grid_depth);
  int max_ranges = 1;
  for (int i = 0; i < effective_grid_depth; i++) {
    int dim = grid_dims_order_[i];
    int start_col = get_column(q_start[dim], dim);
    int end_col = get_column(q_end[dim], dim);
    cur_cols[i] = start_col;
    start_cols[i] = start_col;
    end_cols[i] = end_col;
    if (i < effective_grid_depth - 1) {
        max_ranges *= end_col - start_col + 1;
    }
  }

  Set<PhysicalIndex> ranges;
  ranges.ranges.resize(max_ranges);
  // Number of cells in the last dimension is the group returned.
  int cells_per_range = end_cols[effective_grid_depth-1] - start_cols[effective_grid_depth-1] + 1;
  for (int j = effective_grid_depth; j < grid_dims_order_.size(); j++) {
      auto nc = partition_boundaries_[grid_dims_order_[j]];
      cells_per_range *= nc.size();
  }

  while (cur_cols[0] <= end_cols[0]) {
    int start_cell_ix = get_cell_number(cur_cols);
    int end_cell_ix = start_cell_ix + cells_per_range;
    PhysicalIndex start_pix = cell_boundaries_[start_cell_ix];
    PhysicalIndex end_pix = cell_boundaries_[end_cell_ix];
    ranges.ranges.emplace_back(start_pix, end_pix);
    // Don't need to increment the last item in cur_cols because they're returned as a group.
    for (int i = effective_grid_depth-2; i >= 0; i--) {
        cur_cols[i] += 1;
        if (cur_cols[i] > end_cols[i] && i > 0) {
            cur_cols[i] = start_cols[i];
        } else {
            break;
        }
    }
  }
/*
  // Loop over all hypercells formed by non-uniform grids
#if STATS
  int num_hypercells = 0;
  int num_ranges = 0;
#endif
  bool hypercell_loop_end = false;
  if (n_effective_base_dims == 0) {
    // There is only one hypercell: the entire grid.
    hypercell_loop_end = true;
  }
  while (true) {
    // Set column ranges for non-uniform dims
    bool empty_hypercell = false;
    for (int dim : queried_nonuniform_dims) {
      const NonuniformGrid& nug = nonuniform_grids_[dim];
      int gix = grid_dims_[dim].grid_ix;
      int base_gix = grid_dims_[nug.base_dim].grid_ix;
      int ix = gix - n_uniform_dims_;
      int nix = cur_cols[base_gix] - start_cols[base_gix];  // nested vector ix
      if (secondary_dim_empty[ix][nix]) {
        empty_hypercell = true;
        break;
      }
      std::pair<int, int>& border_cols = secondary_dim_border_cols[ix][nix];
      start_cols[gix] = border_cols.first;
      end_cols[gix] = border_cols.second;
      cur_cols[gix] = start_cols[gix];
      std::pair<int, int>& border_cols_exact = secondary_dim_border_cols_exact[ix][nix];
      start_cols_exact[gix] = border_cols_exact.first;
      end_cols_exact[gix] = border_cols_exact.second;
    };

    // Loop over cells in this hypercell
    if (!empty_hypercell) {
#if STATS
      num_hypercells++;
#endif
      bool cell_loop_end = false;
      while (true) {
#if STATS
        num_ranges++;
#endif
        int cell_ix = get_cell_number(cur_cols);

        bool is_exact = complete_containment;
        for (int i = 0; i < effective_grid_depth; i++) {
          is_exact = is_exact &&
              (cur_cols[i] >= start_cols_exact[i]) &&
              (cur_cols[i] <= end_cols_exact[i]);
        }

        PhysicalIndex start_ix = cell_boundaries_[cell_ix];
        PhysicalIndex end_ix = cell_boundaries_[cell_ix + cells_per_range];
        if (start_ix < end_ix) {
          ranges_old.push_back({cell_ix, start_ix, end_ix, is_exact});
          ranges.ranges.emplace_back(start_ix, end_ix);
        }

        // Next cell
        for (int i = effective_grid_depth - 1; i >= n_effective_base_dims; i--) {
          cur_cols[i]++;
          if (cur_cols[i] <= end_cols[i]) {
            break;
          }
          cur_cols[i] = start_cols[i];
          if (i == n_effective_base_dims) {
            cell_loop_end = true;
            break;
          }
        }
        if (cell_loop_end) break;
      }
    }

    // Next hypercell
    for (int i = n_effective_base_dims - 1; i >= 0; i--) {
      cur_cols[i]++;
      if (cur_cols[i] <= end_cols[i]) {
        break;
      }
      cur_cols[i] = start_cols[i];
      if (i == 0) {
        hypercell_loop_end = true;
        break;
      }
    }
    if (hypercell_loop_end) break;
  }

#if STATS
  LOG("num_projected_hypercells", num_hypercells);
  LOG("num_projected_cells", num_ranges * cells_per_range);
  LOG("num_projected_ranges", num_ranges);
#endif
*/
  return ranges;
}

template <size_t D>
bool FloodIndex<D>::no_intersection(Scalar q_start, Scalar q_end, int dim) const {
  return (q_start > dim_maxs_[dim] || q_end < dim_mins_[dim]);
}

template <size_t D>
bool FloodIndex<D>::full_intersection(Scalar q_start, Scalar q_end, int dim) const {
  return (q_start <= dim_mins_[dim] && q_end >= dim_maxs_[dim]);
}

template <size_t D>
bool FloodIndex<D>::full_intersection(const Query<D>& query, int dim) const {
  if (dim < 0) {
    return true;
  }
  return (query.start[dim] <= dim_mins_[dim] && query.end[dim] >= dim_maxs_[dim]);
}

// Should only be used for initial sorting and processing
template <size_t D>
int FloodIndex<D>::get_cell_number(const Point<D>& pt) {
  int cell_no = 0;
  // Cache column number of base dimensions.
  std::map<int, int> column_cache;
  for (int i = 0; i < n_uniform_dims_; i++) {
    int dim = grid_dims_order_[i];
    const GridDimension& grid_dim = grid_dims_.at(dim);
    int col = get_column(pt[dim], dim);
    cell_no += col * grid_dim.multiplier;
    column_cache[dim] = col;  // we save some extra stuff but that's OK
  }
  for (auto& x : nonuniform_grids_) {
    int dim = x.first;
    NonuniformGrid& nug = x.second;
    const GridDimension& grid_dim = grid_dims_.at(dim);
    int col = get_nonuniform_column(pt[dim], dim, column_cache[nug.base_dim]);
    // Account for inaccuracies in the secondary boundaries because
    // they were computed on a sample.
    if (col < 0) {
      nug.secondary_boundaries[column_cache[nug.base_dim]][0] = pt[dim];
      col = 0;
    } else if (col >= grid_dim.n_cols) {
      nug.secondary_boundaries[column_cache[nug.base_dim]].back() = pt[dim] + 1;
      col = grid_dim.n_cols - 1;
    }
    cell_no += col * grid_dim.multiplier;
  }
  return cell_no;
}

// Input is in order of the grid dims
template <size_t D>
int FloodIndex<D>::get_cell_number(const std::vector<int>& cur_cols) const {
  int cell_no = 0;
  for (int i = 0; i < static_cast<int>(cur_cols.size()); i++) {
    int dim = grid_dims_order_[i];
    const GridDimension& grid_dim = grid_dims_.at(dim);
    cell_no += cur_cols[i] * grid_dim.multiplier;
  }
  return cell_no;
}

// Finds what column a value falls into.
// Only call on uniform dimensions.
template <size_t D>
int FloodIndex<D>::get_column(Scalar dim_val, int dim) const {
//assert(std::find(grid_dims_order_.begin(), grid_dims_order_.end(), dim) - grid_dims_order_.begin() < n_uniform_dims_);
  const std::vector<Scalar>& boundaries = partition_boundaries_.at(dim);
  return std::upper_bound(boundaries.begin(), boundaries.end(), dim_val) - boundaries.begin() - 1;
}

// Finds what column a value falls into.
// Only call on non-uniform dimensions.
// Returns -1 (or N) if value smaller (or larger) than min (or max) val.
template <size_t D>
int FloodIndex<D>::get_nonuniform_column(Scalar dim_val, int dim, int base_col) const {
  assert(nonuniform_grids_.find(dim) != nonuniform_grids_.end());
  const NonuniformGrid& nug = nonuniform_grids_.at(dim);
  const std::vector<Scalar>& boundaries = nug.secondary_boundaries[base_col];
  return std::upper_bound(boundaries.begin(), boundaries.end(), dim_val) - 1 - boundaries.begin();
}

template <size_t D>
PhysicalIndex FloodIndex<D>::binary_search_lower_bound(PhysicalIndex l, PhysicalIndex r, Scalar key) const {
  while (l < r) {
    PhysicalIndex mid =  l + (r - l) / 2;
    if (key <= dataset_->GetCoord(mid, sort_dim_)) {
      r = mid;
    } else {
      l = mid + 1;
    }
  }
  return l;
}

template <size_t D>
PhysicalIndex FloodIndex<D>::binary_search_upper_bound(PhysicalIndex l, PhysicalIndex r, Scalar key) const {
  while (l < r) {
    PhysicalIndex mid =  l + (r - l) / 2;
    if (key >= dataset_->GetCoord(mid, sort_dim_)) {
      l = mid + 1;
    } else {
      r = mid;
    }
  }
  return l;
}

template <size_t D>
std::string FloodIndex<D>::to_string() const {
  std::string str;
  str += "Indexed dims: [ ";
  for (int dim : indexed_dims_) {
    str += std::to_string(dim) + " ";
  }
  str += "], Grid dims: [ ";
  for (int dim : grid_dims_order_) {
    str += std::to_string(dim) + " ";
  }
  str += "], Num cols: [ ";
  for (int dim : grid_dims_order_) {
    const GridDimension& grid_dim = grid_dims_.at(dim);
    str += std::to_string(grid_dim.n_cols) + " ";
  }
  str += "], Functional mappings: [ ";
  for (const auto& x : functional_mappings_) {
    int mapped_dim = x.first;
    int target_dim = x.second.target_dim;
    str += std::to_string(mapped_dim) + "->" + std::to_string(target_dim) + " ";
  }
  str += "], Nonuniform grids: [ ";
  for (const auto& x : nonuniform_grids_) {
    int secondary_dim = x.first;
    int base_dim = x.second.base_dim;
    str += std::to_string(base_dim) + "/" + std::to_string(secondary_dim) + " ";
  }
  str += "]";
  return str;
}

template <size_t D>
size_t FloodIndex<D>::Size() const {
  int partition_boundaries_size = 0;
  for (const auto& x : partition_boundaries_) {
    partition_boundaries_size += x.size() * sizeof(Scalar);
  }
  int nugs_size = 0;
  //for (const auto& x : nonuniform_grids_) {
  //  const NonuniformGrid& nug = x.second;
  //  nugs_size += grid_dims_[nug.base_dim].n_cols * grid_dims_[nug.secondary_dim].n_cols * sizeof(Scalar);
  //}
  return sizeof(FloodIndex<D>) + cell_boundaries_.size() * sizeof(PhysicalIndex) + partition_boundaries_size + nugs_size;
}

template <size_t D>
int FloodIndex<D>::num_cells() {
  int n = 1;
  for (const auto& x : grid_dims_) {
    n *= x.second.n_cols;
  }
  return n;
}

/******** Measuring for optimization ********/

// Loads min/max for each dimension and sets CDF model
template <size_t D>
void FloodIndex<D>::load_metadata(const std::string& metadata_dir) {
  std::string index_file = metadata_dir + "/minmax.bin";
  std::vector<int64_t> content = load_binary_file<int64_t>(index_file);
  assert(content.size() == 2*D);
  int cix = 0;  // cursor index
  for (size_t i = 0; i < D; i++) {
    dim_mins_[i] = content[cix++];
  }
  for (size_t i = 0; i < D; i++) {
    dim_maxs_[i] = content[cix++];
  }
}

// This also initializes the cell boundaries structure.
template <size_t D>
void FloodIndex<D>::process_stats_only(std::vector<Point<D>>& data, bool compute_minmax) {
  if (compute_minmax) {
    // Find mins and maxs
    for (size_t i = 0; i < D; i++) {
      dim_mins_[i] = std::numeric_limits<Scalar>::max();
      dim_maxs_[i] = std::numeric_limits<Scalar>::lowest();
    }
    for (const Point<D> &p : data) {
      for (size_t i = 0; i < D; i++) {
        dim_mins_[i] = std::fmin(p[i], dim_mins_[i]);
        dim_maxs_[i] = std::fmax(p[i], dim_maxs_[i]);
      }
    }
  }

  // Find cell boundaries.
  int n_cells = num_cells();
  std::vector<int> cell_sizes(n_cells, 0);
  for (const Point<D>& pt : data) {
    int cell_no = get_cell_number(pt);
    cell_sizes[cell_no]++;
  }

  cell_boundaries_.reserve(n_cells+1);
  cell_boundaries_.push_back(0);
  for (int i = 0; i < n_cells; i++) {
    cell_boundaries_.push_back(cell_boundaries_[i] + cell_sizes[i]);
  }
}

template <size_t D>
void FloodIndex<D>::query_stats_only(const Query<D> &query, double est_sort_dim_selectivity, bool do_refinement) {
  if (do_refinement) {
    assert(dataset_ != nullptr);
  }

  // Does not include sort dim, because the range over the sort dim will
  // be exact after refinement.
  std::vector<int> query_dimensions;
  bool should_search = false;
  for (int i = 0; i < static_cast<int>(D); i++) {
    if (!full_intersection(query.start[i], query.end[i], i)) {
      if (i == sort_dim_) {
        should_search = true;
      } else {
        query_dimensions.push_back(i);
      }
    }
    if (no_intersection(query.start[i], query.end[i], i)) {
      // Query does not intersect data at all.
      return;
    }
  }

  // Project
#if STATS
  auto start_time = std::chrono::high_resolution_clock::now();
#endif
  // Range ends should be exclusive
  std::vector<FloodIndex<D>::AGRange> ranges = get_query_ranges(query);
#if STATS
  auto finish_time = std::chrono::high_resolution_clock::now();
  auto projector_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
  LOG("projector_time", projector_time);
#endif

  // Rectify
#if STATS
  double avg_log_search_range = 0;
//  double log_avg_search_range = 0;
//  start_time = std::chrono::high_resolution_clock::now();
#endif
  if (should_search && ranges.size() > 0) {
    Scalar sort_dim_min = query.start[sort_dim_];
    Scalar sort_dim_max = query.end[sort_dim_];
    for (FloodIndex<D>::AGRange& range : ranges) {
#if STATS
      avg_log_search_range += std::log2(range.end_ix - range.start_ix);
//      log_avg_search_range += range.end_ix - range.start_ix;
#endif
      if (do_refinement) {
        // If we want to include this then we need to enable the dataset
        range.start_ix = binary_search_lower_bound(range.start_ix, range.end_ix, sort_dim_min);
        range.end_ix = binary_search_upper_bound(range.start_ix, range.end_ix, sort_dim_max);
      }
    }
#if STATS
    avg_log_search_range /= static_cast<double>(ranges.size());
//    log_avg_search_range = std::log2(log_avg_search_range / static_cast<double>(ranges.size()));
#endif
  }
#if STATS
//  finish_time = std::chrono::high_resolution_clock::now();
//  auto rectifier_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
//  LOG("rectifier_time", rectifier_time);
  LOG("avg_log_search_range", avg_log_search_range);
//  LOG("log_avg_search_range", log_avg_search_range);
#endif

  // Scan
#if STATS
  size_t visited_points = 0;
//  size_t valid_points = 0;
  size_t exact_visited_points = 0;
  int num_scanned_ranges = 0;
  int num_scanned_exact_ranges = 0;
//  start_time = std::chrono::high_resolution_clock::now();
#endif
  for (const FloodIndex<D>::AGRange& range : ranges) {
    if (range.end_ix <= range.start_ix) {
      continue;
    }
#if STATS
    visited_points += range.end_ix - range.start_ix;
    num_scanned_ranges++;
#endif
    if (range.exact) {
#if STATS
      num_scanned_exact_ranges++;
//      valid_points += range.end_ix - range.start_ix;
      exact_visited_points += range.end_ix - range.start_ix;
#endif
//      visitor.visitExactRange(dataset_, range.start_ix, range.end_ix);
    } else {
//      for (PhysicalIndex p = range.start_ix; p < range.end_ix; p += 64) {
//        PhysicalIndex true_end = std::min(range.end_ix, p + 64);
//        // A way to get the last true_end - p bits set to 1.
//        uint64_t valids = 1ULL + (((1ULL << (true_end - p - 1)) - 1ULL) << 1);
//        for (int dim : query_dimensions) {
//          valids &= dataset_->GetCoordRange(p, true_end, dim, query.start[dim], query.end[dim]);
//        }
//#if STATS
//        valid_points += __builtin_popcountll(valids);
//#endif
//        visitor.visitRange(dataset_, p, true_end, valids);
//      }
    }
  }
#if STATS
//  finish_time = std::chrono::high_resolution_clock::now();
//  auto scan_time = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time-start_time).count();
//  LOG("scan_time", scan_time);

  LOG("exact_visited_points", exact_visited_points * est_sort_dim_selectivity);
//  LOG("visited_points", visited_points);
//  LOG("valid_points", valid_points);
  LOG("num_ranges", ranges.size());
  LOG("num_searched_ranges", should_search ? ranges.size() : 0 );
  LOG("num_scanned_ranges", num_scanned_ranges);
  LOG("num_scanned_exact_ranges", num_scanned_exact_ranges);
//  LOG("single_scans", visited_points * query_dimensions.size());
//  LOG("exact_single_scans", exact_visited_points * query_dimensions.size());
  LOG("inexact_single_scans", (visited_points - exact_visited_points) * query_dimensions.size() * est_sort_dim_selectivity);
#endif
}


#pragma once

#include <vector>
#include <memory>
#include <string>

#include "primary_indexer.h"
#include "dataset.h"
#include "types.h"

template <size_t D>
class FloodIndex : public PrimaryIndexer<D> {
 public:

  explicit FloodIndex(const std::string& index_dir);
  virtual bool Init(PointIterator<D> start, PointIterator<D> end) override;
    
  void SetDataset(std::shared_ptr<Dataset<D>> dataset) override;

  Set<PhysicalIndex> IndexRanges(Query<D>& query) override;
  size_t Size() const override;

private:
  bool no_intersection(Scalar q_start, Scalar q_end, int dim) const;
  bool full_intersection(Scalar q_start, Scalar q_end, int dim) const;
  bool full_intersection(const Query<D>& query, int dim) const;

  int get_cell_number(const Point<D>& pt);
  int get_cell_number(const std::vector<int>& cur_cols) const;
  int get_column(Scalar dim_val, int dim) const;
  int get_nonuniform_column(Scalar dim_val, int dim, int base_col) const;
  PhysicalIndex binary_search_lower_bound(PhysicalIndex l, PhysicalIndex r, Scalar key) const;
  PhysicalIndex binary_search_upper_bound(PhysicalIndex l, PhysicalIndex r, Scalar key) const;

  void SortAndProcess(std::vector<Point<D>>& data);
  void SortAndProcess(std::vector<Point<D>>& data, PhysicalIndex start_offset, PhysicalIndex end_offset);
  std::string to_string() const;

  int num_cells();

  // Query only to gather stats (does not actually scan and visit).
  // Used when optimizing.
  void query_stats_only(const Query<D>& query, double est_sort_dim_selectivity, bool do_refinement=false);
  void load_metadata(const std::string& metadata_dir);
  void process_stats_only(std::vector<Point<D>>& data, bool compute_minmax);

  struct AGRange {
    // The cell index for this range.
    // An index < 0 indicates an empty cell - do not rectify it.
    int cell_ix;

    // The actual range. Will be computed.
    PhysicalIndex start_ix;  // inclusive
    PhysicalIndex end_ix;  // exclusive

    // True if the range doesn't have to filtered once the actual
    // physical indices are found. This saves scanning time.
    bool exact;
  };
  
  struct FunctionalMapping {
    int mapped_dim;  // the dim taken out of the index
    int target_dim;
    double a;  // slope
    double b;  // intercept
    double left_error_bound;  // should be negative
    double right_error_bound;

    Scalar map_lower_bound(Scalar val) const {
      // Technically this might overshoot the error bound by 1.
      return static_cast<Scalar>(a * val + b + left_error_bound);
    }

    Scalar map_upper_bound(Scalar val) const {
      return static_cast<Scalar>(a * val + b + right_error_bound);
    }
  };

  struct NonuniformGrid {
    int base_dim;
    int secondary_dim;  // the dim whose boundaries are non-uniform
    // If secondary_dim has N columns, each boundary vector has N+1 elements
    std::vector<std::vector<Scalar>> secondary_boundaries;
  };

  // Does not include mapped dimensions and sort dimension
  struct GridDimension {
    int dim;  // dim in original dataset
    int grid_ix;  // order this appears in the grid
    int n_cols;
    int multiplier;  // used when computing cell number
  };

  struct AGVirtualIndex {
    int cell_no;
    Scalar sort_val;
  };

  int sort_dim_;

  // Dims of the original dataset that are indexed
  std::vector<int> indexed_dims_;
  // Dims of the original dataset, in the order they appear in the grid
  // The grid does not include mapped dimensions and the sort dimension
  std::vector<int> grid_dims_order_;
  // Key is dim of original dataset
  std::map<int, GridDimension> grid_dims_;
  int n_grid_dims_;

  // Key is mapped dim.
  std::map<int, FunctionalMapping> functional_mappings_;

  // Key is secondary dim.
  std::map<int, NonuniformGrid> nonuniform_grids_;
  // Base dims come before all other dims.
  int n_base_dims_;
  // Uniform dims (including base dims) come before non-uniform dims.
  int n_uniform_dims_;

  // Set during sorting and processing
  Scalar dim_mins_[D];
  Scalar dim_maxs_[D];

  std::vector<PhysicalIndex> cell_boundaries_;
  std::array<std::vector<Scalar>, D> partition_boundaries_;

  std::shared_ptr<Dataset<D>> dataset_;
  PhysicalIndex start_offset_ = 0;
};

#include "../src/flood_index.hpp"

#pragma once

#include <vector>
#include <map>
#include <memory>

#include "primary_indexer.h"
#include "types.h"

/*
 * An outlier index is one that partitions the data into non-outliers and outliers, and may have
 * separate indexes (or none) on each.
 */
template <size_t D>
class OutlierIndex : public PrimaryIndexer<D> {
  public:
    OutlierIndex(const std::vector<size_t>& outlier_list);

    virtual bool Init(PointIterator<D> start, PointIterator<D> end) override;

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override; 
   
    // Sets the indexer for the non-outliers, must be called *before* Init
    void SetIndexer(std::unique_ptr<PrimaryIndexer<D>> indexer);

    // Sets the indexer for the outliers, optional but must be called *before* Init.
    void SetOutlierIndexer(std::unique_ptr<PrimaryIndexer<D>> indexer);

    size_t Size() const override {
        size_t s = main_indexer_->Size();
        if (outlier_indexer_ != NULL) {
            s += outlier_indexer_->Size();
        }
        // Add in the size of the outliers, since we're assuming that we copy them for each
        // correlation that exists.
        //s += (data_size_ - outlier_start_ix_) * sizeof(Point<D>);
        return s;
    }

  private:
    std::vector<size_t> outlier_list_;
    // True when the index has been initialized.
    bool ready_;
    // The index of the dimension this index is built on.
    size_t column_;
    // The bucket number we reserve for outliers.
    const Scalar outlier_bucket_ = SCALAR_MAX;
    // The number of points in the dataset.
    size_t data_size_;
    // The index of the first outlier in the dataset. All indexes after this correspond to outliers.
    size_t outlier_start_ix_;
    // The index on the non-outliers (required)
    std::unique_ptr<PrimaryIndexer<D>> main_indexer_;
    // The index on the outliers (optional)
    std::unique_ptr<PrimaryIndexer<D>> outlier_indexer_;
    
};

#include "../src/outlier_index.hpp"

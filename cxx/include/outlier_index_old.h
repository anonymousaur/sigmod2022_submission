#pragma once

#include <vector>
#include <map>

#include "indexer.h"
#include "types.h"

template <size_t D>
class SingleDimensionalOutlierIndex : public Indexer<D> {
  public:
    struct Page {
        // Inclusive range
        std::pair<Scalar, Scalar> value_range;
        // Exclusive of the upper range bound.
        std::pair<size_t, size_t> index_range;
        Pair() : value_range(0, 0), index_range(0, 0) {}
    };

    SingleDimensionalOutlierIndex(size_t dim, size_t page_size,
            const std::vector<size_t>& outlier_list);

    virtual void Init(std::vector<Point<D>>::iterator start,
            std::vector<Point<D>>::iterator end) override;


    std::vector<PhysicalIndexRange> Ranges(const Query<D>& q) const override; 
    
    size_t Size() const override {
        return (buckets_.size() + 1) * sizeof(PhysicalIndexRange);
    }

  private:
    // Returns the index range that spans the minimum set of pages that must be accessed to retrieve
    // the given range [start, end] (inclusive).
    PhysicalIndexRange PageRangeFor(Scalar start, Scalar end);
    
    // When building the index, assuming a working page p, and a new candidate points with the same
    // value, determines whether to truncate the page and add it to the index, or start a new one.
    void ExtendOrTruncPage(Page *p, size_t minix, size_t maxix); 
    
    std::vector<size_t> outlier_list_;
    // True when the index has been initialized.
    bool ready_;
    // The index of the dimension this index is built on.
    size_t column_;
    // The bucket number we reserve for outliers.
    const Scalar outlier_bucket_ = SCALAR_MAX;
    // A map from dimension value to index range where points with that dimension value that are NOT
    // outliers can be found.
    std::map<Scalar, Page> pages_;
    // The range of data indices corresponding to outliers.
    PhysicalIndexRange outlier_range_;
    // The maxmimum number of unique values that fit onto a single page.
    // In other words, as many points will be packed into a page such that either:
    // - all points have the same value of column_
    // - there are no more than page_size_ points per page.
    // Therefore, a page size of 1 indicates
    // that each value can be accessed exactly (use that for categorical vars).
    size_t page_size_;
    
};

#include "../src/outlier_index.hpp"

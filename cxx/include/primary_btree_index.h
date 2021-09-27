#pragma once

#include <vector>
#include <map>

#include "primary_indexer.h"
#include "types.h"

template <size_t D>
class PrimaryBTreeIndex : public PrimaryIndexer<D> {
  public:
    struct Page {
        // Inclusive range
        std::pair<Scalar, Scalar> value_range;
        // Exclusive of the upper range bound.
        std::pair<size_t, size_t> index_range;
        Page() : value_range(0, 0), index_range(0, 0) {}
        Page(std::pair<Scalar, Scalar> vr, std::pair<size_t, size_t> ir) 
            : value_range(vr), index_range(ir) {}
    };

    PrimaryBTreeIndex(size_t dim, size_t page_size);

    virtual bool Init(PointIterator<D> start, PointIterator<D> end) override;

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override; 
    
    size_t Size() const override {
        return (pages_.size() + 1) * sizeof(Page);
    }

    std::vector<Page> Pages() const {
        std::vector<Page> pages;
        for (auto it = pages_.cbegin(); it != pages_.cend(); it++) {
            pages.push_back(it->second);
        }
        return pages;
    }
    
    // Returns the index range that spans the minimum set of pages that must be accessed to retrieve
    // the given range [start, end] (inclusive).
    // Note: public for testing.
    Range<PhysicalIndex> PageRangeFor(Scalar start, Scalar end) const;

  private:
    // Number of data points
    size_t data_size_;
    
    // When building the index, assuming a working page p, and new candidate points with the same
    // value, determines whether to truncate the page and add it to the index, or start a new one.
    void ExtendOrTruncPage(Page *p, Scalar val, size_t minix, size_t maxix); 

    // Used for internal purposes only.
    size_t column_;    
    // True when the index has been initialized.
    bool ready_;
    // A map from dimension value to index range where points with that dimension value that are NOT
    // outliers can be found.
    std::map<Scalar, Page> pages_;
    // The maxmimum number of unique values that fit onto a single page.
    // In other words, as many points will be packed into a page such that either:
    // - all points have the same value of column_
    // - there are no more than page_size_ points per page.
    // Therefore, a page size of 1 indicates
    // that each value can be accessed exactly (use that for categorical vars).
    size_t page_size_;
    
};

#include "../src/primary_btree_index.hpp"

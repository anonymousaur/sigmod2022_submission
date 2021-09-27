#pragma once

#include <vector>

#include "indexer.h"
#include "dataset.h"
#include "types.h"

/*
 * An index that just sorts the data along the given column but doesn't evaluate
 * queries. Used for the outlier indexer.
 */
template <size_t D>
class JustSortIndex : public PrimaryIndexer<D> {
  public:
    JustSortIndex(size_t dim) : PrimaryIndexer<D>(dim), column_(dim) {}

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override {
        List<PhysicalIndex> lst;
        return Set<PhysicalIndex>({{0, data_size_}}, lst);
    }

    void Init(PointIterator<D> start, PointIterator<D> end) override;

    size_t Size() const override {
        return 2*sizeof(size_t);
    }


  protected:
    size_t column_;
    size_t data_size_; 

};

#include "../src/just_sort_index.hpp" 


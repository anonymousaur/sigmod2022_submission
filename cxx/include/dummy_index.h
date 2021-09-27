#pragma once

#include <iterator>
#include <algorithm>
#include <vector>

#include "types.h"
#include "dataset.h"
#include "primary_indexer.h"

template <size_t D>
class DummyIndex : public PrimaryIndexer<D> {
    public:

    DummyIndex<D>() : PrimaryIndexer<D>(), data_size_() {};    
        
    // Given a query bounding box, specified by the bottom left point p1 and
    // bottom-right point p2, initialize an iterator, which successively returns
    // ranges of VirtualIndices to check.
    virtual Set<PhysicalIndex> IndexRanges(Query<D>& q) override { 
        return Set<PhysicalIndex>({{0, data_size_}}, List<PhysicalIndex> {});
    }

    virtual bool Init(PointIterator<D> start, PointIterator<D> end) override {
        data_size_ = std::distance(start, end);
        return false;
    }

    // Size of the indexer in bytes
    virtual size_t Size() const override {
        return 0;
    }
    
    virtual std::unordered_set<size_t> GetColumns() const { return this->columns_; }

    void WriteStats(std::ofstream& statsfile) {
        statsfile << "primary_index_type: dummy_index" << std::endl;
    }

    protected:
    size_t data_size_;
};

#pragma once

#include <iterator>
#include <algorithm>
#include <vector>

#include "types.h"
#include "dataset.h"
#include "indexer.h"

template <size_t D>
using ConstPointIterator = typename std::vector<Point<D>>::const_iterator;

template <size_t D>
class CorrelationIndexer : public Indexer<D> {
    public:

    CorrelationIndexer<D>() : column_() {};    
    CorrelationIndexer<D>(size_t dim) : column_(dim) {};
        
    // Given a query bounding box, specified by the bottom left point p1 and
    // bottom-right point p2, initialize an iterator, which successively returns
    // ranges of VirtualIndices to check.
    virtual Set<Key> KeyRanges(const Query<D>& query) const = 0;

    virtual void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) = 0;

    // Size of the indexer in bytes
    virtual size_t Size() const override = 0;

    virtual size_t GetMappedColumn() const { return column_; }    

    IndexerType Type() const override { return IndexerType::Correlation; }

    virtual void Insert(const std::vector<InsertRecord<D>>& inserts) {} 
    virtual void DummyInsert(const std::vector<InsertRecord<D>>& inserts) { Insert(inserts); } 

    virtual void SetDataset(std::shared_ptr<Dataset<D>>) { // Do nothing
    }

    protected:
    // This may not be set on construction, but must be set after Init returns. 
    size_t column_;    
};


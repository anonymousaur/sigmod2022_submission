#pragma once

#include <iterator>
#include <algorithm>
#include <vector>

#include "types.h"
#include "dataset.h"
#include "indexer.h"


template <size_t D>
class PrimaryIndexer : public Indexer<D> {
    public:

    PrimaryIndexer<D>() : columns_() {};    
    PrimaryIndexer<D>(size_t dim) : columns_({dim}) {};
        
    virtual void SetDataset(std::shared_ptr<Dataset<D>> dataset) {};
    
    // Called during query execution to return the ranges of indexes that should be accessed for
    // this query. The set of indexes is a superset of the true matching records.
    virtual Set<PhysicalIndex> IndexRanges(Query<D>& query) = 0;

    // Sorts the data according to this primary index. Returns true if the data was modified.
    virtual bool Init(PointIterator<D> start, PointIterator<D> end) = 0;

    // Size of the indexer in bytes
    virtual size_t Size() const override = 0;
    
    virtual std::unordered_set<size_t> GetColumns() const { return columns_; }

    IndexerType Type() const override { return IndexerType::Primary; }

    virtual std::vector<InsertRecord<D>> Insert(const std::vector<Point<D>>& points) {
        AssertWithMessage(false, "Insert is unimplemented");
        return {};
    }
    
    virtual std::vector<InsertRecord<D>> DummyInsert(const std::vector<Point<D>>& points,
            const std::vector<size_t>& indexes) {
        AssertWithMessage(false, "DummyInsert is unimplemented");
        return {};
    }

    virtual Ranges<Key> KeyRangesForBuckets(std::vector<int32_t> bucket_ids) {
        AssertWithMessage(false, "KeyRangesForBuckets is not implemented");
        return {};
    }

    protected:
    // This may not be set on construction, but must be set after Init returns. 
    std::unordered_set<size_t> columns_;    
};

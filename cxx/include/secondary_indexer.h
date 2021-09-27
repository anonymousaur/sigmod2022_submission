#pragma once

#include <iterator>
#include <algorithm>
#include <vector>
#include <optional>

#include "indexer.h"
#include "types.h"
#include "dataset.h"
   
/*
 * Interface for a secondary index, which does not determine the layout of the data. Instead of
 * returning matching ranges, it can only return the indexes of the points that match.
 */
template <size_t D>
class SecondaryIndexer : public Indexer<D> {
  public:
    SecondaryIndexer<D>(size_t dim) { column_ = dim; }
      
    // Given a query bounding box, specified by the bottom left point p1 and
    // bottom-right point p2, initialize an iterator, which successively returns
    // ranges of VirtualIndices to check.
    // An optional without a value means this index cannot filter the query (all indexes are valid)
    virtual List<Key> Matches(const Query<D>& query) const = 0;

    virtual void Init(ConstPointIterator<D> start,
            ConstPointIterator<D> end) = 0;

    // Size of the indexer in bytes
    virtual size_t Size() const override = 0;

    virtual size_t GetColumn() const { return column_; }

    IndexerType Type() const override { return IndexerType::Secondary; }

    // Inserts the given indices into the secondary index, and shifts all other indexes
    // appropriately. Call either this OR Notify, not both.
    virtual void Insert(const std::vector<KeyPair>& inserts) {}
    virtual void DummyInsert(const std::vector<KeyPair>& inserts) { Insert(inserts); }
    virtual void Remove(const ScalarRange&, const Range<Key>&) {};
    virtual void Insert(const std::vector<InsertRecord<D>>& records) {
        std::vector<KeyPair> kps;
        kps.reserve(records.size());
        for (auto& r : records) {
            kps.emplace_back(r.point[column_], r.inserted_index);
        }
        Insert(kps);
    }
    virtual void DummyInsert(const std::vector<InsertRecord<D>>& records) {
        Insert(records);
    }

    protected:
    size_t column_;
};

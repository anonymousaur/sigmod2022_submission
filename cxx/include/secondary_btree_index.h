#pragma once

#include <vector>
#include <map>

#include "cpp-btree/btree_map.h"
#include "secondary_indexer.h"
#include "types.h"

template <size_t D>
class SecondaryBTreeIndex : public SecondaryIndexer<D> {
  public:
    SecondaryBTreeIndex(size_t dim);

    // NOTE: this will fail once the key is not just the index
    // TODO(vikram): Fix so that it can take in any key.
    void SetIndexList(List<Key> list) {
        index_subset_ = list;
        use_index_subset_ = true;
    }

    void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override;

    List<Key> Matches(const Query<D>& q) const override; 
    
    size_t Size() const override {
        return btree_.bytes_used();
        /*
        size_t unique_keys = 0;
        size_t total_entries = btree_.size();
        Scalar prev_key = std::numeric_limits<Scalar>::lowest();
        for (auto it = btree_.begin(); it != btree_.end(); it++) {
            assert (it->first >= prev_key);
            if (it->first > prev_key) {
                unique_keys++;
                prev_key = it->first;
            }
        }
        // Based on: https://code.google.com/archive/p/cpp-btree/
        // Each key (in a random insertion benchmark) carries an overhead of 10 bytes (for 64-bit
        // machines)
        return unique_keys*10 + total_entries*sizeof(PhysicalIndex);*/
    }

    void Insert(const std::vector<KeyPair>& inserts) override;
    void Remove(const ScalarRange& value_range, const Range<Key> key_range);

  private:
    // Number of data points
    size_t data_size_;
    // True when the index has been initialized.
    bool ready_;
    bool use_index_subset_;
    List<Key> index_subset_;
    // A map from dimension value to index range where points with that dimension value that are
    // outliers can be found.
    btree::btree_multimap<Scalar, Key> btree_;
};

#include "../src/secondary_btree_index.hpp"

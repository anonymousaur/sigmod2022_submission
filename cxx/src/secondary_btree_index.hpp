#include "secondary_btree_index.h"

#include <iostream>

template <size_t D>
SecondaryBTreeIndex<D>::SecondaryBTreeIndex(size_t dim)
    : SecondaryIndexer<D>(dim), use_index_subset_(false), btree_() {}

template <size_t D>
List<Key> SecondaryBTreeIndex<D>::Matches(const Query<D>& q) const {
    List<Key> idxs;
    auto filter = q.filters[this->column_];
    assert (filter.present);
    if (filter.is_range) {
        for (ScalarRange r : filter.ranges) {
            auto startit = btree_.lower_bound(r.first);
            auto endit = btree_.upper_bound(r.second);
            for (auto it = startit; it != endit; it++) {
                idxs.push_back(it->second);
            }
        }
    }
    else {
        for (Scalar val : filter.values) {
            auto startit = btree_.lower_bound(val);
            auto endit = btree_.upper_bound(val);
            for (auto it = startit; it != endit; it++) {
                idxs.push_back(it->second);
            }
        }
    }
    std::cout << "Returning " << idxs.size() << " indexes from secondary B+ tree" << std::endl;
    // Leave these unsorted for now. Anyone using them can sort if necessary.
    return idxs;
}

template <size_t D>
void SecondaryBTreeIndex<D>::Init(ConstPointIterator<D> start, ConstPointIterator<D> end) {
    data_size_ = std::distance(start, end);
    if (!use_index_subset_) {
        size_t i = 0;
        std::cout << "No index subset provided; indexing all points" << std::endl;
        for (auto it = start; it != end; it++, i++) {
            btree_.insert(std::make_pair((*it)[this->column_], (Key)i));
        }
    } else {
        std::cout << "Using provided index list to load B+ tree" << std::endl;
        for (size_t ix : index_subset_) {
            btree_.insert(std::make_pair((*(start+ix))[this->column_], (Key)ix));
        }
        index_subset_.clear();
    }
    std::cout << "SecondaryBTreeIndex on " << this->column_ << " loaded " << btree_.size() << " points"
        << " and total size " << Size() << std::endl;
}

template <size_t D>
void SecondaryBTreeIndex<D>::Insert(const std::vector<KeyPair>& inserts) {
    btree_.insert(inserts.begin(), inserts.end());    
}

template <size_t D>
void SecondaryBTreeIndex<D>::Remove(const ScalarRange& scalar_range, const Range<Key> value_range) {
    auto lb = btree_.lower_bound(scalar_range.first);
    auto rb = btree_.lower_bound(scalar_range.second);
    // Deleting while iterating invalidates the above iterators, so use size as an indicator for
    // when we're done.
    size_t s = std::distance(lb, rb);
    auto it = lb;
    for (size_t i = 0; i < s; i++) {
        if (it->second >= value_range.start && it->second < value_range.end) {
            it = btree_.erase(it);
        } else {
            it++;
        }
    }
}


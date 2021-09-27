#include "outlier_index.h"

#include <algorithm>
#include <parallel/algorithm>
#include <cassert>
#include <unordered_map>
#include <string>
#include <iostream>

#include "types.h"

template <size_t D>
OutlierIndex<D>::OutlierIndex(const std::vector<size_t>& outlier_list)
    : PrimaryIndexer<D>(0), main_indexer_(), outlier_indexer_(), outlier_list_(outlier_list) {}

template <size_t D>
void OutlierIndex<D>::SetIndexer(std::unique_ptr<PrimaryIndexer<D>> indexer) {
    assert (indexer);
    main_indexer_ = std::move(indexer);
}

template <size_t D>
void OutlierIndex<D>::SetOutlierIndexer(std::unique_ptr<PrimaryIndexer<D>> indexer) {
    assert (indexer);
    outlier_indexer_ = std::move(indexer);
}

template <size_t D>
Set<PhysicalIndex> OutlierIndex<D>::IndexRanges(Query<D>& q) {
    bool relevant = false;
    for (size_t col : this->columns_) {
        relevant |= q.filters[col].present;
    }
    // If the primary indexer is useless, just return the full range. Since outliers are small,
    // there's not much the outlier index will be able to do.
    if (!relevant) {
        std::cout << "Outlier index not relevant" << std::endl;
        return Set<PhysicalIndex>({{0, data_size_}}, List<PhysicalIndex>());
    }
    auto ix_set = main_indexer_->IndexRanges(q);
    if (outlier_indexer_) {
        // Add the outlier ranges, but since they are relative to the outlier start index (i.e.,
        // they aren't aware that outliers are shifted), they have to be shifted manually.
        auto outlier_ix_set = outlier_indexer_->IndexRanges(q);
        ix_set.ranges.reserve(ix_set.ranges.size() + outlier_ix_set.ranges.size());
        for (Range<PhysicalIndex> r : outlier_ix_set.ranges) {
            ix_set.ranges.emplace_back(r.start + outlier_start_ix_, r.end + outlier_start_ix_);
        }
        ix_set.list.reserve(ix_set.list.size() + outlier_ix_set.list.size());
        for (PhysicalIndex p : outlier_ix_set.list) {
            ix_set.list.push_back(p + outlier_start_ix_);
        }
    } else if (outlier_start_ix_ < data_size_) {
        ix_set.ranges.emplace_back(outlier_start_ix_, data_size_);
    }
    return ix_set;
}

template <size_t D>
bool OutlierIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    std::sort(outlier_list_.begin(), outlier_list_.end());
    // Move all the outliers to the end of the list.
    auto next_outlier = end-1;
    for (auto oit = outlier_list_.rbegin(); oit != outlier_list_.rend(); oit++) {
        size_t oix = *oit;
        if (start + oix != next_outlier) {
            std::iter_swap(start + oix, next_outlier);
        }
        next_outlier--;
    }
    auto outlier_start = next_outlier + 1;

    bool modified = false;    
    assert (main_indexer_);
    modified |= main_indexer_->Init(start, outlier_start);
    data_size_ = std::distance(start, end);
    outlier_start_ix_ = data_size_ - outlier_list_.size(); 
    if (outlier_indexer_) {
       modified |= outlier_indexer_->Init(outlier_start, end);
    }
    
    // Clear the list since we don't need it anymore.
    outlier_list_.clear();
    auto main_cols = main_indexer_->GetColumns();
    this->columns_.insert(main_cols.begin(), main_cols.end());
    ready_ = true;
    return modified;
}


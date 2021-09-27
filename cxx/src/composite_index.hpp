#include "composite_index.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>

#include "merge_utils.h"

template <size_t D>
CompositeIndex<D>::CompositeIndex(size_t gap)
    : PrimaryIndexer<D>(), gap_threshold_(gap), secondary_indexes_() {}


template <size_t D>
bool CompositeIndex<D>::SetPrimaryIndex(std::unique_ptr<PrimaryIndexer<D>> index) {
    assert (index != NULL);
    primary_index_ = std::move(index);
    return true;
}

template <size_t D>
bool CompositeIndex<D>::AddSecondaryIndex(std::unique_ptr<SecondaryIndexer<D>> index) {
    AssertWithMessage(index != NULL, "Tried to add NULL secondary index");
    AssertWithMessage(correlation_indexes_.empty(), "Cannot use secondary index with correlation index"); 
    AssertWithMessage(rewriters_.empty(), "Cannot use secondary index with rewriters"); 
    secondary_indexes_.push_back(std::move(index));
    return true;
}

template <size_t D>
bool CompositeIndex<D>::AddCorrelationIndex(std::unique_ptr<CorrelationIndexer<D>> index) {
    AssertWithMessage(index != NULL, "Tried to add NULL correlation index");
    AssertWithMessage(secondary_indexes_.empty(), "Cannot use correlation index with secondary index"); 
    AssertWithMessage(rewriters_.empty(), "Cannot use correlation index with rewriters");
    correlation_indexes_.push_back(std::move(index));
    return true;
}

template <size_t D>
bool CompositeIndex<D>::AddRewriter(std::unique_ptr<Rewriter<D>> rewriter) {
    AssertWithMessage(rewriter != NULL, "Tried to add NULL rewriter");
    AssertWithMessage(secondary_indexes_.empty(), "Cannot use rewriters with secondary index"); 
    AssertWithMessage(correlation_indexes_.empty(), "Cannot use rewriters with correlation index"); 
    rewriters_.push_back(std::move(rewriter));
    return true;
}

template <size_t D>
bool CompositeIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    AssertWithMessage(primary_index_ != NULL, "Primary index was not set");
    bool modified = primary_index_->Init(start, end);
    auto cols = primary_index_->GetColumns();
    this->columns_.insert(cols.begin(), cols.end());
    for (auto& ci : correlation_indexes_) {
        ci->Init(start, end);
        this->columns_.insert(ci->GetMappedColumn());
    }
    for (auto& si : secondary_indexes_) {
        si->Init(start, end);
        this->columns_.insert(si->GetColumn());
    }
    for (auto& rw : rewriters_) {
        rw->Init(start, end);
    }
    data_size_ = std::distance(start, end);
    return modified;
}

template <size_t D>
List<Key> CompositeIndex<D>::RangesWithRewriter(Query<D>& q) {
    List<Key> auxiliary_indexes;
    // Assumes all the indexes from rewriters are unsorted.
    for (auto& rw : rewriters_) {
        List<Key> aux = rw->Rewrite(q);
        if (auxiliary_indexes.empty() && !aux.empty()) {
            auxiliary_indexes = std::move(aux);
        } else {
            auxiliary_indexes.insert(auxiliary_indexes.end(), aux.begin(), aux.end());
        }
    }
    std::sort(auxiliary_indexes.begin(), auxiliary_indexes.end());
    return auxiliary_indexes; 
}

template <size_t D>
Set<Key> CompositeIndex<D>::RangesWithCorrelation(Query<D>& q) {
    Set<Key> res;
    bool first_scan = true;
    for (auto& ci : correlation_indexes_) {
        if (!q.filters[ci->GetMappedColumn()].present) {
            continue;
        }
        Set<Key> ixs = ci->KeyRanges(q);
        res = first_scan ? std::move(ixs) : MergeUtils::Intersect<Key>(res, ixs);
        first_scan = false;
    }
    return res;
}

template <size_t D>
List<Key> CompositeIndex<D>::RangesWithSecondary(Query<D>& q) {
    // For each secondary index, merge the secondary index matches into it.
    List<Key> matches;
    // This is to make sure we sort the secondary index result only when we absolutely have to.
    bool needs_sort = true;
    std::cout << "Getting matches from secondary index" << std::endl;
    for (auto& si : secondary_indexes_) {
        if (!q.filters[si->GetColumn()].present) {
            continue;
        }
        List<Key> next_matches = si->Matches(q);
        if (matches.empty()) {
            // Don't sort yet - wait until there other other matches or ranges that need
            // intersecting.
            matches = std::move(next_matches);
        } else {
            auto start = std::chrono::high_resolution_clock::now();
            if (needs_sort) {
                std::sort(matches.begin(), matches.end());
                needs_sort = false;
            }
            auto mid = std::chrono::high_resolution_clock::now();
            std::sort(next_matches.begin(), next_matches.end());
            // Only sort if we absolutely have to
            matches = MergeUtils::Intersect<Key>(matches, next_matches);
            auto end = std::chrono::high_resolution_clock::now();
            auto tt1 = std::chrono::duration_cast<std::chrono::nanoseconds>(mid - start).count();
            auto tt2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end - mid).count();
            std::cout << "Sort time: " << tt1 << "ns, "
                << "Intersect time: " << tt2 << "ns" << std::endl;
        }
    }
    if (needs_sort) {
        std::sort(matches.begin(), matches.end());
    }
    std::cout << "Returning matches from secondary index" << std::endl;
    return matches;
}

template <size_t D>
Set<PhysicalIndex> CompositeIndex<D>::IndexRanges(Query<D>& q) {
    // lookups is guaranteed to not have duplicates.
    if (!rewriters_.empty()) {
        List<Key> lookups = RangesWithRewriter(q);
        Set<PhysicalIndex> primary = primary_index_->IndexRanges(q);
        Set<PhysicalIndex> aux_matches = dataset_->Lookup(Set<Key>({}, lookups));
        AssertWithMessage(aux_matches.ranges.size() == 0, "Got ranges while running rewriter");
        return MergeUtils::Union<PhysicalIndex>(primary.ranges, aux_matches.list); 
    } else if (!correlation_indexes_.empty()) {
        auto t0 = std::chrono::high_resolution_clock::now();
        Set<Key> lookups = RangesWithCorrelation(q);
        auto t1 = std::chrono::high_resolution_clock::now();
        Set<PhysicalIndex> primary = primary_index_->IndexRanges(q);
        auto t2 = std::chrono::high_resolution_clock::now();
        Set<PhysicalIndex> aux_matches = dataset_->Lookup(lookups);
        auto t3 = std::chrono::high_resolution_clock::now();
        auto corr_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
        auto primary_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
        auto resolve_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t3-t2).count();
        std::cout << "Correlation Index Time: " << corr_time / 1e6 << std::endl;
        std::cout << "Primary Index Lookup Time: " << primary_time / 1e6 << std::endl;
        std::cout << "Clustered Resolution Time: " << resolve_time / 1e6 << std::endl;
         
        std::cout << "Composite: primary index returned " << primary.ranges.size()
            << " ranges and " << primary.list.size() << " points" << std::endl;
        std::cout << "Composite after dataset lookup: correlation index returned " << lookups.ranges.size()
            << " ranges and " << lookups.list.size() << " points" << std::endl;
        bool full_scan = primary.ranges.size() == 1 &&
                    primary.ranges[0].end - primary.ranges[0].start == data_size_;
        if (full_scan) {
            return aux_matches;
        } else {
            return MergeUtils::Intersect<PhysicalIndex>(primary, aux_matches);
        }
    } else if (!secondary_indexes_.empty()) {
        List<Key> lookups = RangesWithSecondary(q);
        std::cout << "Starting primary index lookup" << std::endl;
        Set<PhysicalIndex> primary = primary_index_->IndexRanges(q);
        std::cout << "Got primary index results" << std::endl;
        bool full_scan = primary.ranges.size() == 1 &&
                    primary.ranges[0].end - primary.ranges[0].start == data_size_;
        std::cout << "Starting lookup" << std::endl;
        Set<PhysicalIndex> aux_matches = dataset_->Lookup(Set<Key>({}, lookups));
        std::cout << "Lookup finished" << std::endl;
        if (full_scan) {
            return aux_matches;
        } else {
            return MergeUtils::Intersect<PhysicalIndex>(primary, aux_matches);
        }
    } else {
        return primary_index_->IndexRanges(q);
    }
}

template <size_t D>
std::vector<InsertRecord<D>> CompositeIndex<D>::Insert(const std::vector<Point<D>>& points) {
    auto start = std::chrono::high_resolution_clock::now();
    auto records = primary_index_->Insert(points);
    auto mid = std::chrono::high_resolution_clock::now();
    // Pass to each of the other indexes.
    for (auto& ci : correlation_indexes_) {
        ci->Insert(records);
    }
    for (auto& si : secondary_indexes_) {
        si->Insert(records);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto primary_time = std::chrono::duration_cast<std::chrono::nanoseconds>(mid-start).count();
    auto update_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-mid).count();
    std::cout << "Primary index update time: " << primary_time / 1e6 << std::endl;
    std::cout << "Correlation index update time: " << update_time / 1e6 << std::endl;
    // TODO: rewriters
    return records;
} 

template <size_t D>
std::vector<InsertRecord<D>> CompositeIndex<D>::DummyInsert(const std::vector<Point<D>>& points,
        const std::vector<size_t>& indexes) {
    // don'e actually change the underlying data storage.
    auto records = primary_index_->DummyInsert(points, indexes);
    // Pass to each of the other indexes.
    for (auto& ci : correlation_indexes_) {
        ci->DummyInsert(records);
    }
    for (auto& si : secondary_indexes_) {
        si->DummyInsert(records);
    }
    // TODO: rewriters
    return records;
} 

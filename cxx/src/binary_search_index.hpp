#include "binary_search_index.h"

#include <algorithm>
#include <parallel/algorithm>
#include <cassert>
#include <string>
#include <iostream>

#include "types.h"
#include "utils.h"
#include "file_utils.h"

template <size_t D>
BinarySearchIndex<D>::BinarySearchIndex(size_t dim)
    : BinarySearchIndex<D>(dim, "") {}

template <size_t D>    
BinarySearchIndex<D>::BinarySearchIndex(size_t dim, const std::string& target_bucket_filename) :PrimaryIndexer<D>(dim), column_(dim), host_buckets_(), ready_(false) {
    if (!target_bucket_filename.empty()) {
        Load(target_bucket_filename);
    }
}

template <size_t D>
size_t BinarySearchIndex<D>::LocateLeft(Scalar val) const {
    size_t l = 0, r = dataset_->Size() - 1;
    while (l < r) {
        size_t m = (l + r)/2;
        if (dataset_->GetCoord(m, column_) < val) {
            l = m+1;
        } else {
            r = m;
        }
    }
    return l;
}

template <size_t D>
size_t BinarySearchIndex<D>::LocateRight(Scalar val) const {
    size_t l = 0, r = dataset_->Size() - 1;
    while (l < r) {
        size_t m = (l + r)/2;
        if (dataset_->GetCoord(m, column_) <= val) {
            l = m+1;
        } else {
            r = m;
        }
    }
    return l;
}

template <size_t D>
Set<PhysicalIndex> BinarySearchIndex<D>::IndexRanges(Query<D>& q) {
    // Querying 
    auto accessed = q.filters[column_];
    if (!accessed.present) {
        return {.ranges = {{0, dataset_->Size()}}, .list = List<PhysicalIndex>()};
    }
    Ranges<PhysicalIndex> ranges;
    if (accessed.is_range) {
        ranges.reserve(accessed.ranges.size());
        size_t added = 0;
        for (ScalarRange r : accessed.ranges) {
            size_t lix = LocateLeft(r.first);
            size_t rix = LocateRight(r.second);
            if (added > 0 && lix == ranges[added-1].end) {
                ranges[added-1].end = rix;
            } else if (rix > lix) {
                ranges.emplace_back(lix, rix);
                added++;
            }
        }
    } else {
        ranges.reserve(accessed.values.size());
        size_t added = 0;
        for (Scalar val : accessed.values) {
            size_t lix = LocateLeft(val);
            size_t rix = LocateRight(val);
            if (added > 0 && lix == ranges[added-1].end) {
                ranges[added-1].end = rix;
            } else if (rix > lix) {
                ranges.emplace_back(lix, rix);
                added++;
            }
        }
    }
    return Set<PhysicalIndex>(ranges, List<PhysicalIndex>());
}

template <size_t D>
bool BinarySearchIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    size_t s = std::distance(start, end);
    std::vector<std::pair<Scalar, size_t>> indices(s);
    size_t i = 0;
    for (auto it = start; it != end; it++, i++) {
        Scalar ix_val = (*it)[column_];
        indices[i] = std::make_pair(ix_val, i);
    }

    // Sort by this array instead, preserving the existing order as much as possible.
    std::stable_sort(indices.begin(), indices.end(),
        [](const std::pair<Scalar, size_t>& a, const std::pair<Scalar, size_t>& b) -> bool {
            return a.first < b.first;
        });
    bool data_modified = false;
    for (size_t i = 0; i < indices.size(); i++) {
        if (i != indices[i].second) {
            data_modified = true;
        }
    }

    std::vector<Point<D>> data_cpy(s);
    const auto start_tmp = start;
    std::transform(indices.begin(), indices.end(), data_cpy.begin(),
            [start](const auto& ix_pair) -> Point<D> {
                return *(start + ix_pair.second);
                });

    std::copy(data_cpy.cbegin(), data_cpy.cend(), start);
    // Validate the host buckets
    if (host_buckets_.size() > 0) {
        auto host_it = host_buckets_.begin();
        size_t ix = 0;
        bool valid;
        for (auto it = start; it < end; it++, ix++) {
            while ((*it)[column_] >= host_it->EndValue()) {
                host_it++;
            }
            valid &= ix >= host_it->StartOffset() && ix < host_it->EndOffset();
        }
        //AssertWithMessage(valid, "Target bucket file does NOT match data"); 
    }
    if (data_modified) {
        std::cout << "BinarySearchIndex modified data" << std::endl;
    } else {
        std::cout << "BinarySearchIndex did not modify data" << std::endl;
    }
    ready_ = true;
    return data_modified;
}

template <size_t D>
void BinarySearchIndex<D>::Load(const std::string& filename) {
    std::ifstream file(filename);
    AssertWithMessage(file.is_open(), "Could not open file " + filename);
    
    auto header = FileUtils::NextArray<std::string>(file, 3);
    //AssertWithMessage(std::stoi(header[1]) == column_, "Target file column does not match preset");
    size_t s = std::stoi(header[2]);
    size_t prev_end = 0;
    host_buckets_.reserve(s);
    for (size_t i = 0; i < s; i++) { 
        auto arr = FileUtils::NextArray<size_t>(file, 5);
        if (host_buckets_.size() > 0) {
            prev_end = host_buckets_.back().EndOffset();
        }
        host_buckets_.emplace_back(i, arr[3], arr[4]);
        // We will verify these at initialization.
        //host_buckets_.back().start_index = 0;
        //host_buckets_.back().end_index = 0;
        host_buckets_.back().start_index = (size_t)arr[1];
        host_buckets_.back().end_index = (size_t)arr[2];
        AssertWithMessage(prev_end == host_buckets_.back().start_index,
                "Bucket start (" + std::to_string(host_buckets_.back().start_index) +
                " does not match previous bucket end (" + std::to_string(prev_end) + ")");
    }
    AssertWithMessage(std::is_sorted(host_buckets_.begin(), host_buckets_.end(), [] (const auto& lhs, const auto& rhs) {
            return lhs.start_index < rhs.start_index;
            }), "Target buckets indexes not sorted");
    AssertWithMessage(std::is_sorted(host_buckets_.begin(), host_buckets_.end(), [] (const auto& lhs, const auto& rhs) {
            return lhs.start_value < rhs.start_value;
            }), "Target buckets values not sorted");
    std::cout << "Loaded BinarySearchIndex targets file" << std::endl;
    std::cout << "BinarySearchIndex added " << host_buckets_.size() << " host buckets" << std::endl;
}

template <size_t D>
std::vector<InsertRecord<D>> BinarySearchIndex<D>::Insert(const std::vector<Point<D>>& new_pts) {
    AssertWithMessage(host_buckets_.size() > 0, "Cannot insert without host buckets");
    std::vector<InsertRecord<D>> records;
    records.reserve(new_pts.size());
    InsertData<D> insert_data;
    insert_data.reserve(new_pts.size());
    for (const auto& p : new_pts) {
        size_t loc = LocateRight(p[column_]);
        insert_data.emplace_back(p, loc);
    }
    dataset_->Insert(insert_data);
    auto it = host_buckets_.begin();
    size_t total_inserted = 0;
    size_t per_bucket_inserted = 0;
    // The inserts come back sorted.
    for (const auto& ins : insert_data) {
        while (ins.second >= it->EndValue()) {
            it->start_index += total_inserted;
            total_inserted += per_bucket_inserted;
            it->end_index += total_inserted;
            per_bucket_inserted = 0;
            it++;
        }
        records.emplace_back(ins.first, ins.second, &*it);
        per_bucket_inserted++;
    }
    // Shift offsets for the remainder of host buckets.
    for (; it < host_buckets_.end(); it++) {
        it->start_index += total_inserted;
        total_inserted += per_bucket_inserted;
        it->end_index += total_inserted;
        per_bucket_inserted = 0;
    }
    return records;
}

template <size_t D>
std::vector<InsertRecord<D>> BinarySearchIndex<D>::DummyInsert(const std::vector<Point<D>>& new_pts, const std::vector<size_t>& indexes) {
    AssertWithMessage(new_pts.size() == indexes.size(), "Points and indexes differ in size");
    std::vector<InsertRecord<D>> records;
    records.reserve(new_pts.size());
    InsertData<D> insert_data;
    insert_data.reserve(new_pts.size());
    for (size_t i = 0; i < new_pts.size(); i++) {
        insert_data.emplace_back(new_pts[i], indexes[i]);
    }
    //dataset_->Insert(insert_data);
    size_t col = column_;
    std::sort(insert_data.begin(), insert_data.end(), [col] (const auto& lhs, const auto& rhs) {
            return lhs.first[col] < rhs.first[col];
            });
    auto it = host_buckets_.begin();
    // The inserts come back sorted.
    for (const auto& ins : insert_data) {
        while (ins.first[column_] >= it->EndValue()) {
            it++;
        }
        records.emplace_back(ins.first, ins.second, &*it);
    }
    return records;
}


//template <size_t D>
//Ranges<Key> BinarySearchIndex<D>::KeyRangesForBuckets(std::vector<int32_t> bucket_ids) {
//    Ranges<Key> ranges;
//    ranges.reserve(bucket_ids.size());
//    int32_t prev_r = 0;
//    auto it = host_buckets_.begin();
//    size_t total_size = 0;
//    for (int32_t r : bucket_ids) {
//        while (it->Id() < r) { it++;}
//        total_size += it->EndOffset() - it->StartOffset();
//        if (ranges.size() > 0 && r == prev_r+1) {
//            ranges.back().end = (Key)(it->EndOffset());
//        } else {
//            ranges.push_back(Range<Key>{it->StartOffset(), it->EndOffset()});
//        }
//        prev_r = r;
//        it++;
//    }
//
//}

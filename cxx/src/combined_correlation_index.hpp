#include "combined_correlation_index.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <memory>

#include "utils.h"
#include "file_utils.h"
#include "types.h"

template <size_t D>
Set<Key> CombinedCorrelationIndex<D>::KeyRanges(const Query<D>& q) const {
    auto start = std::chrono::high_resolution_clock::now();
    Ranges<Key> mapped_ranges = mapped_index_->KeyRanges(q);
    auto mid = std::chrono::high_resolution_clock::now();
    auto mid2 = mid;
    std::cout << "## forcing time: " << mapped_ranges.size() << std::endl;
    Set<Key> ret;
    if (outlier_index_) {
        List<Key> lst = outlier_index_->Matches(q);
        // TODO(vikram): whether this is sorted depends on if we're using
        // bucketedSecondaryIndexer.
        std::sort(lst.begin(), lst.end());
        std::cout << "## forcing time: " << (lst.empty() || lst[0] > 1000) << std::endl;
        mid2 = std::chrono::high_resolution_clock::now();
        ret = MergeUtils::Union(mapped_ranges, lst);
    } else {
        ret = Set<Key>(mapped_ranges, {});
    }
    std::cout << "After merging with outlier index, have " << ret.ranges.size()
        << " ranges and " << ret.list.size() << " items " << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    auto range_t = std::chrono::duration_cast<std::chrono::nanoseconds>(mid-start).count();
    auto match_t = std::chrono::duration_cast<std::chrono::nanoseconds>(mid2-mid).count();
    auto merge_t = std::chrono::duration_cast<std::chrono::nanoseconds>(end-mid2).count();
    std::cout << "Range time (us): " << range_t / 1e3 << std::endl;
    std::cout << "Outlier match time (us): " << match_t / 1e3 << std::endl;
    std::cout << "Merge time (us): " << merge_t / 1e3 << std::endl;
    return ret;
}

template <size_t D>
void CombinedCorrelationIndex<D>::ProcessTrackerDiffs(const DiffMap& diffs) {
    std::vector<KeyPair> outliers;
    size_t new_inlier = 0, new_outlier = 0, remain_inlier = 0, remain_outlier = 0,
           inlier_to_outlier = 0, outlier_to_inlier = 0;
    for (auto it = diffs.begin(); it != diffs.end(); it++) {
        // No grouping right now. Could be made more efficient.
        for (const Action& action : it->second) {
            int32_t map_bucket = it->first;
            int32_t host_bucket = action.target_bucket_index;
            switch (action.diff_type) {
                case NEW_INLIER:
                    new_inlier += 1;
                    mapped_index_->AddBucket(host_bucket, map_bucket);
                    inlier_buckets_ += 1;
                    break;
                case NEW_OUTLIER:
                    new_outlier += 1;
                    outliers.insert(outliers.end(), action.indexes.begin(), action.indexes.end());
                    // outlier_index_->Insert(action.indexes);
                    outlier_buckets_ += 1;
                    outlier_points_ += action.indexes.size();
                    break;
                case REMAIN_INLIER:
                    remain_inlier += 1;
                    // No action needed;
                    break;
                case REMAIN_OUTLIER:
                    remain_outlier += 1;
                    //outlier_index_->Insert(action.indexes);
                    outliers.insert(outliers.end(), action.indexes.begin(), action.indexes.end());
                    break;
                case OUTLIER_TO_INLIER:
                    outlier_to_inlier += 1;
                    outlier_index_->Remove(mapped_index_->GetBucket(map_bucket), 
                        {(Key)(host_buckets_[host_bucket]->StartOffset()),
                            (Key)(host_buckets_[host_bucket]->EndOffset())});
                    mapped_index_->AddBucket(host_bucket, map_bucket);
                    break;
                case INLIER_TO_OUTLIER:
                    inlier_to_outlier += 1;
                    // The indexes *include* the ones that exist already and need to be transferred.
                    mapped_index_->RemoveBucket(host_bucket, map_bucket);
                    //outlier_index_->Insert(action.indexes);
                    outliers.insert(outliers.end(), action.indexes.begin(), action.indexes.end());
                    break;
                default:
                    AssertWithMessage(false, "Got NO_DIFF");
            }
        }
    }
    std::sort(outliers.begin(), outliers.end(), [] (const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
            });
    outlier_index_->Insert(outliers);
    std::cout << "Done processing tracker diffs" << std::endl;
    std::cout << "Got new outliers: " << new_outlier << std::endl
        << "new inlier: " << new_inlier << std::endl
        << "remain inlier: " << remain_inlier << std::endl
        << "remain_outlier: " << remain_outlier << std::endl
        << "inlier_to_outlier: " << inlier_to_outlier << std::endl
        << "outlier_to_inlier: " << outlier_to_inlier << std::endl;
}

template <size_t D>
void CombinedCorrelationIndex<D>::Insert(const std::vector<InsertRecord<D>>& records) {
    // Records are already sorted. Group by target bucket.
    int prev_id = std::numeric_limits<int32_t>::lowest();
    std::vector<KeyPair> values_to_insert;
    std::cout << "In CombinedCorrelationIndex::Insert" << std::endl;
    for (const auto r : records) {
        int32_t next_id = r.host_bucket->Id();
        AssertWithMessage(next_id >= prev_id, "Inserts to CombinedCorrelationIndex are out of order");
        if (next_id > prev_id && !values_to_insert.empty()) {
            tracker_->InsertBatchList(std::make_pair(prev_id, r.host_bucket), values_to_insert);
            host_buckets_[prev_id] = r.host_bucket;
            values_to_insert.clear();
        }
        prev_id = next_id;
        values_to_insert.push_back(std::make_pair(r.point[this->column_], r.inserted_index));
    }
    if (values_to_insert.size() > 0) {
        tracker_->InsertBatchList(std::make_pair(prev_id, records.back().host_bucket), values_to_insert);
        host_buckets_[prev_id] = records.back().host_bucket;
    }
    std::cout << "In CombinedCorrelationIndex::Insert, begin ProcessTrackerDiffs" << std::endl;
    ProcessTrackerDiffs(tracker_->Diffs());
    tracker_->ResetDiffs();
}

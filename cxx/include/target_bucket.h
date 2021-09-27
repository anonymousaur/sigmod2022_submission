#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

#include "utils.h"

struct MapBucket {
    // The index of this mapped bucket as determined by the map_bucket_ map.
    int32_t index;
    // The number of points in this bucket.
    int32_t num_points;
    // Whether this bucket has been stashed as an outlier.
    // TODO(vikram): reduce space by packing this with num_points.
    bool is_outlier;
};
    

struct TargetBucket {
    // List of MapBuckets sorted in increasing order by size.
    std::vector<MapBucket> buckets_;
    // We need to access individual buckets too, so this map points to their location in the
    // map_bucket array.
    std::map<int32_t, uint32_t> map_bucket_index_;
    // The index of the first inlier bucket in the buckets_ array.
    int32_t first_inlier_index_;
    uint32_t num_inlier_points_;
    uint32_t total_points_;
    uint32_t num_inlier_buckets_;
    float storage_factor_;
    // The amount over / under the threshold we can go without changing from inlier to outlier or
    // vice versa, as a fraction of the total number of points in the bucket.
    float tolerance_;
    float beta_;
    // Record how each map bucket was changed.
    std::map<int32_t, DiffType> diffs_;

    TargetBucket(float alpha, float beta) : buckets_(),
                    map_bucket_index_(),
                    first_inlier_index_(0),
                    num_inlier_points_(0),
                    num_inlier_buckets_(0),
                    tolerance_(0.0),
                    total_points_(0),
                    storage_factor_(alpha),
                    beta_(beta), diffs_() {}
    // Recompute from scratch which points should be inliers and outliers.

    size_t Size() {
        // Account for the fact that we can bin these together.
        return buckets_.size() * (sizeof(MapBucket)-4);
    }

    uint32_t Find(int32_t map_bucket_id) {
        auto loc = map_bucket_index_.find(map_bucket_id);
        AssertWithMessage(loc != map_bucket_index_.end(), "Map bucket not present");
        return loc->second;
    }

    // Document that a new bucket was created that didn't exist before.
    void RecordNewBucket(MapBucket& mb) {
        diffs_[mb.index] = mb.is_outlier ? NEW_OUTLIER : NEW_INLIER;
    }
    
    // This documents when an existing bucket had points added to it.
    void RecordInsert(MapBucket& mb) {
        diffs_[mb.index] = mb.is_outlier ? REMAIN_OUTLIER : REMAIN_INLIER;
    }

    // This documents whenever something switches from inlier to outlier or vice versa.
    void RecordSwitch(MapBucket& mb) {
        DiffType previous = diffs_[mb.index];
        if (mb.is_outlier) {
            if (previous == NEW_INLIER) { diffs_[mb.index] = NEW_OUTLIER; }
            else if (previous == NO_DIFF || previous == REMAIN_INLIER) {
                diffs_[mb.index] = INLIER_TO_OUTLIER;
            }
        } else {
            if (previous == NEW_OUTLIER) { diffs_[mb.index] = NEW_INLIER; }
            else if (previous == NO_DIFF || previous == REMAIN_OUTLIER) {
                diffs_[mb.index] = OUTLIER_TO_INLIER;
            }
        }
    }

    std::vector<std::pair<int32_t, DiffType>> Diffs() {
        std::vector<std::pair<int32_t, DiffType>> ret(diffs_.begin(), diffs_.end());
        return ret;
    }

    void ResetDiffs() {
        diffs_.clear();
    }
    
    // Whether or not to stash this bucket. < 0 means definitely stash, = 0 means don't bother
    // changing the status right now, and > 0 means make it an inlier.
    float CostToStash(const MapBucket& mb) const {
        //int32_t other_inlier_pts = num_inlier_points_ - (mb.is_outlier ? 0 : mb.num_points);
        //int32_t other_inlier_buckets = num_inlier_buckets_ - (mb.is_outlier ? 0 : 1);
        //// Overhead incurred if an outlier.
        //float outlier_cost = storage_factor_ * mb.num_points;
        //// Overhead incurred if an inlier. There are two contributions. If a point is an inlier, it
        //// incurs the scan overhead of everything else in that bucket, and all other buckets incur the
        //// scan overhead of it.
        //float inlier_cost = other_inlier_pts + (other_inlier_buckets * mb.num_points);
        float inlier_cost = total_points_;
        float outlier_cost = (beta_ + storage_factor_)*mb.num_points; 
        float ret = 0;
        ret += (outlier_cost > (1+tolerance_)*inlier_cost);
        ret -= (outlier_cost < (1-tolerance_)*inlier_cost);
        return ret;
    }
    
    void Swap(uint32_t bucket_ix1, uint32_t bucket_ix2) {
        int32_t id1 = buckets_[bucket_ix1].index;
        int32_t id2 = buckets_[bucket_ix2].index;
        auto loc = map_bucket_index_.find(id1);
        loc->second = bucket_ix2;
        loc = map_bucket_index_.find(id2);
        loc->second = bucket_ix1;
        std::swap(buckets_[bucket_ix1], buckets_[bucket_ix2]);
    }

    void Realign(int32_t internal_bucket_ix) {
        bool outlier = buckets_[internal_bucket_ix].is_outlier;
        while (internal_bucket_ix < buckets_.size() - 1 &&
                buckets_[internal_bucket_ix+1].num_points < buckets_[internal_bucket_ix].num_points) {
            if (outlier) {
                MakeOutlier(buckets_[internal_bucket_ix+1]);
            }
            Swap(internal_bucket_ix, internal_bucket_ix+1);
            internal_bucket_ix++;
        }
        while (internal_bucket_ix > 0 &&
                buckets_[internal_bucket_ix].num_points < buckets_[internal_bucket_ix-1].num_points) {
            if (buckets_[internal_bucket_ix-1].is_outlier) {
                MakeOutlier(buckets_[internal_bucket_ix]);
            }
            Swap(internal_bucket_ix, internal_bucket_ix-1);
            internal_bucket_ix--;
        }
        Restash();
    }

    // Add `npts` points to the given bucket.
    void AddPoints(int32_t map_bucket_id, int32_t npts) {
        total_points_ += npts;
        MapBucket mb = {.index = map_bucket_id, .num_points = npts, .is_outlier = true};
        auto loc = map_bucket_index_.find(map_bucket_id);
        if (loc == map_bucket_index_.end()) {
            auto it = std::upper_bound(buckets_.begin(), buckets_.end(), mb,
                    [] (const MapBucket& mb1, const MapBucket& mb2) {
                        return mb1.num_points < mb2.num_points;
                    });
            
            map_bucket_index_.emplace(map_bucket_id, std::distance(buckets_.begin(), it));
            if (it != buckets_.begin()) {
                if (!((it-1)->is_outlier)) {
                    mb.is_outlier = false;
                    num_inlier_points_ += npts;
                    num_inlier_buckets_ += 1;
                }
            }
            it = buckets_.insert(it, mb);
            RecordNewBucket(mb);    
            it++;
            // Increment the remaining bucket indexes.
            // TODO(vikram): insertion is easy with a linked list instead of a vector. 
            for ( ; it != buckets_.end(); it++) {
                map_bucket_index_[it->index]++;
            }    
            // This takes care of fixing first_inlier_index_
            Restash();
        } else {
            MapBucket& mb = buckets_[loc->second];
            mb.num_points += npts;
            if (!mb.is_outlier) {
                num_inlier_points_ += npts;
            }
            RecordInsert(mb);
            Realign(loc->second);
        }
    }

    // Each element is a pair of (map bucket id, num points).
    // When this batch method is called, the entire target bucket is resorted,
    // not inserted individually.
    void AddPointsBatch(std::vector<std::pair<int32_t, int32_t>> buckets) {
        for (const auto& b : buckets) {
            auto loc = map_bucket_index_.find(b.first);
            if (loc == map_bucket_index_.end()) {
                MapBucket mb = {.index = b.first, .num_points = b.second, .is_outlier = true};
                buckets_.push_back(mb);
                RecordNewBucket(mb);
            } else {
                buckets_[loc->second].num_points += b.second;
                RecordInsert(buckets_[loc->second]);
            }
            total_points_ += b.second;
        }
        // Sort by increasing size.
        std::sort(buckets_.begin(), buckets_.end(), 
                [](const auto& lhs, const auto& rhs) -> bool {
                    return lhs.num_points < rhs.num_points;
                });
        map_bucket_index_.clear();
        num_inlier_points_ = 0;
        num_inlier_buckets_ = 0;
        // Build the map_bucket_index_ map from scratch.
        // Reset every bucket to be an outlier, and compute inliers from scratch.
        for (size_t i = 0; i < buckets_.size(); i++) {
            map_bucket_index_[buckets_[i].index] = i;
            buckets_[i].is_outlier = true;
        }
        first_inlier_index_ = buckets_.size();
        Restash();
    }

    void MakeInlier(MapBucket& mb) {
        if (mb.is_outlier) {
            mb.is_outlier = false;
            num_inlier_buckets_++;
            num_inlier_points_ += mb.num_points;
            RecordSwitch(mb);
        }
    }

    void MakeOutlier(MapBucket& mb) {
        if (!mb.is_outlier) {
            mb.is_outlier = true;
            num_inlier_buckets_--;
            num_inlier_points_ -= mb.num_points;
            RecordSwitch(mb);
        }
    }
    
    void Restash() {
        // Traverse until we find the boundary between outliers and inliers.
        first_inlier_index_ = std::max(first_inlier_index_, 0);
        while (first_inlier_index_ < buckets_.size() &&
                CostToStash(buckets_[first_inlier_index_]) < 0) {
            MakeOutlier(buckets_[first_inlier_index_]);
            first_inlier_index_++;
        }
        first_inlier_index_ = std::min(first_inlier_index_, (int32_t)(buckets_.size())-1);
        while (first_inlier_index_ >= 0 &&
                CostToStash(buckets_[first_inlier_index_]) > 0) {
            MakeInlier(buckets_[first_inlier_index_]);
            first_inlier_index_--;
        }
        // Assuming first_inlier_index_ is in bounds, we end up with first_inlier_index_ one less than
        // where we want it.
        first_inlier_index_ = std::max(0, std::min((int32_t)(buckets_.size()), first_inlier_index_+1));
    }

    // This is not meant to be called regularly, as it is slow.
    // Just use it during development to ensure the target buckets are internally consistent.
    bool Consistent() const {
        // Check consistency of index mapping
        for (auto it = map_bucket_index_.begin(); it != map_bucket_index_.end(); it++) {
            int32_t map_ix = it->first;
            int32_t bucket_ix = buckets_[it->second].index;
            if (map_ix != bucket_ix) {
                std::cerr << "ERROR: target bucket index mapping is inconsistent. "
                    << "map_bucket_index_[" << map_ix << "] = " << it->second
                    << ", which maps to bucket with index " << bucket_ix << std::endl;
                return false;
            }
        }
        // Check consistency of inlier / outlier assignment
        for (const auto& mb : buckets_) {
            float cost = CostToStash(mb);
            if (cost < 0 && !mb.is_outlier) {
                std::cerr << "Expected bucket " << mb.index << " to be an outlier, "
                   << " but got marginal cost " << cost << std::endl;
                return false;
            } else if (cost > 0 && mb.is_outlier) {
                std::cerr << "Expected bucket " << mb.index << " to be an inlier, "
                   << " but got marginal cost " << cost << std::endl;
                return false;
            }
        }
        return true;
    }
};


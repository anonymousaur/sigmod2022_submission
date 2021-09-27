#include "correlation_tracker.h"

#include "utils.h"
#include "file_utils.h"

template <size_t D>
CorrelationTracker<D>::CorrelationTracker() : CorrelationTracker<D>("") {}

    template <size_t D>
CorrelationTracker<D>::CorrelationTracker(const std::string& mapping_file)
    : column_(), storage_weight_(201.3), beta_(17.88), map_buckets_(), target_buckets_() {
    if (!mapping_file.empty()) {
        Load(mapping_file);
    }
}

template <size_t D>
void CorrelationTracker<D>::Load(const std::string& mapping_file) {
    std::ifstream file(mapping_file);
    assert (file.is_open());
    AssertWithMessage(FileUtils::NextLine(file) == "continuous-0", "Wrong file heading");
    
    auto header = FileUtils::NextArray<std::string>(file, 3);
    assert (header[0] == "source");
    column_ = std::stoi(header[1]);
    size_t s = std::stoi(header[2]);
    // The last element is the rightmost bound on the data.
    // Read everything as a double to be safe and then convert to scalar
    
    std::cout << "CorrelationTracker: reading " << s << " mapped buckets" << std::endl;
    for (size_t i = 0; i < s; i++) {
        auto arr = FileUtils::NextArray<double>(file, 3);
        size_t mapix = (size_t)arr[0];
        ScalarRange r = {(Scalar)arr[1], (Scalar)arr[2]};
        map_buckets_.emplace(r.first, mapix);
    }
    file.close();
}

template <size_t D>
int32_t CorrelationTracker<D>::MapBucketFor(Scalar val) {
    auto loc = map_buckets_.upper_bound(val);
    AssertWithMessage(loc != map_buckets_.begin(), "Point " + std::to_string(val) + " found outside existing domain");
    loc--;
    return loc->second;
}

template <size_t D>
void CorrelationTracker<D>::InsertWithTargetBucketSplit(const TargetNode& original,
        const std::vector<TargetNode>& splits) {
    auto orig_loc = target_buckets_.find(original.first);
    assert (orig_loc != target_buckets_.end());
    TargetBucket tb = orig_loc->second;
    target_buckets_.erase(orig_loc);
    for (const auto& new_bucket : splits) {
        auto loc = target_buckets_.find(new_bucket.first);
        AssertWithMessage(loc == target_buckets_.end(), "Split added a bucket with an existing ID");
        TargetBucket tb(storage_weight_, beta_);
        target_buckets_.emplace(new_bucket.first, tb);
        InsertEntireBucket(new_bucket);
    }
}

template <size_t D>
void CorrelationTracker<D>::InsertBatchList(const TargetNode& target,
        const std::vector<KeyPair>& values) {
    // None of these insertions resulted in a target bucket split.
    std::map<int32_t, int32_t> added_pts;
    std::map<int32_t, List<KeyPair>> map_buckets;
    for (const KeyPair& kp : values) {
        int32_t map_ix = MapBucketFor(kp.first);
        added_pts[map_ix]++;
        map_buckets[map_ix].push_back(kp);
    }
    auto loc = target_buckets_.find(target.first);
    if (loc == target_buckets_.end()) {
        loc = target_buckets_.emplace(target.first, TargetBucket(storage_weight_, beta_)).first;
    }
    TargetBucket& tb = loc->second;
    std::vector<std::pair<int32_t, int32_t>> batch_pts(added_pts.begin(), added_pts.end());
    tb.AddPointsBatch(batch_pts);
    MergeDiffs(target, tb.Diffs(), map_buckets);
    tb.ResetDiffs();
}

template <size_t D>
void CorrelationTracker<D>::InsertEntireBucket(const TargetNode& target) {
    std::vector<Scalar> vals;
    std::map<int32_t, List<KeyPair>> map_buckets;
    std::map<int32_t, int32_t> added_pts;
    vals.reserve(target.second->EndOffset() - target.second->StartOffset());
    for (size_t p = target.second->StartOffset(); p < target.second->EndOffset(); p++) {
        Scalar val = dataset_->GetCoord(p, column_);
        int32_t map_ix = map_buckets_.upper_bound(val)->second;
        added_pts[map_ix]++;
        map_buckets[map_ix].push_back(std::make_pair(val, (Key)p));
    }
    auto loc = target_buckets_.find(target.first);
    if (loc == target_buckets_.end()) {
        loc = target_buckets_.emplace(target.first, TargetBucket(storage_weight_, beta_));
    }
    TargetBucket& tb = loc->second;
    std::vector<std::pair<int32_t, int32_t>> batch_pts(added_pts.begin(), added_pts.end());
    tb.AddPointsBatch(batch_pts);
    MergeDiffs(target, tb.Diffs(), map_buckets);
    tb.ResetDiffs();
}

template <size_t D>
void CorrelationTracker<D>::MergeDiffs(const TargetNode& target,
        const std::vector<std::pair<int32_t, DiffType>>& diffs,
        std::map<int32_t, List<KeyPair>>& map_buckets) {
    std::map<int32_t, List<KeyPair>> bucket_scan;
    int32_t target_bucket_ix = target.first;
    for (const auto& diff : diffs) {
        Action a;
        switch (diff.second) {
            case NEW_INLIER:
                a = {.diff_type = NEW_INLIER, .target_bucket_index = target_bucket_ix,
                    .indexes={}};
                diffs_[diff.first].push_back(a);
                break;
            case NEW_OUTLIER:
                a = {.diff_type = NEW_OUTLIER, .target_bucket_index = target_bucket_ix,
                    .indexes = map_buckets[diff.first]};
                AssertWithMessage(a.indexes.size() > 0, "New outlier bucket with no points");
                diffs_[diff.first].push_back(a);
                break;
            case REMAIN_INLIER:
                a = {.diff_type = REMAIN_INLIER, .target_bucket_index = target_bucket_ix,
                    .indexes = {}};
                diffs_[diff.first].push_back(a);
                break;
            case REMAIN_OUTLIER:
                a = {.diff_type = REMAIN_OUTLIER, .target_bucket_index = target_bucket_ix,
                    .indexes = map_buckets[diff.first]};
                AssertWithMessage(a.indexes.size() > 0, "REMAIN_OUTLIER bucket with no new points");
                diffs_[diff.first].push_back(a);
                break;
            case INLIER_TO_OUTLIER:
                // This is a special case - we have to scan the entire bucket to find the indexes
                // that we need to move
                bucket_scan[diff.first] = {};
                break;
            case OUTLIER_TO_INLIER:
                a = {.diff_type = OUTLIER_TO_INLIER, .target_bucket_index = target_bucket_ix,
                    .indexes = {}};
                diffs_[diff.first].push_back(a);
                break;
            default:
                AssertWithMessage(false, "Unrecognized Diff type encountered");
        }
    }
    
    // Now, scan the whole bucket for the remaining buckets that we have to stash entirely.
    if (!bucket_scan.empty()) {
        std::vector<Scalar> values;
        values.reserve(target.second->EndOffset() - target.second->StartOffset());
        for (PhysicalIndex p = target.second->StartOffset(); p < target.second->EndOffset(); p+= 64UL) {
            size_t true_end = std::min(target.second->EndOffset(), p + 64UL);
            // A way to get the last true_end - p bits set to 1.
            uint64_t valids = 1ULL + (((1ULL << (true_end - p - 1)) - 1ULL) << 1);
            dataset_->GetRangeValues(p, true_end, column_, valids, &values);
        }
        size_t ix = target.second->StartOffset(); 
        for (Scalar val : values) {    
            int32_t map_ix = MapBucketFor(val);
            auto loc = bucket_scan.find(map_ix);
            if (loc != bucket_scan.end()) {
                loc->second.push_back(std::make_pair(val, (Scalar)ix));
            }
            ix++;
        }
        size_t moved = 0;
        for (auto it = bucket_scan.begin(); it != bucket_scan.end(); it++) {
            Action a = {.diff_type = INLIER_TO_OUTLIER, .target_bucket_index = target.first,
                .indexes = std::move(it->second)};
            moved += a.indexes.size();
            diffs_[it->first].push_back(a);
        }
    } 
}




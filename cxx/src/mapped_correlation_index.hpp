#include "mapped_correlation_index.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "types.h"
#include "file_utils.h"
#include "merge_utils.h"

template <size_t D>
MappedCorrelationIndex<D>::MappedCorrelationIndex(const std::string& mapping_filename,
        const std::string& target_buckets_filename) 
    : mapping_(), mapping_lst_(), mapping_file_(mapping_filename) {
    Load(mapping_filename, target_buckets_filename);
}

template <size_t D>
void MappedCorrelationIndex<D>::Load(const std::string& mapping_filename, 
        const std::string& target_buckets_filename) {
    std::ifstream file(mapping_filename);
    AssertWithMessage(file.is_open(), "file not found: " + mapping_filename);
    AssertWithMessage(FileUtils::NextLine(file) == "continuous-0", "Bad input file " + mapping_filename);
    
    auto header = FileUtils::NextArray<std::string>(file, 3);
    AssertWithMessage(header[0] == "source", "Bad input file " + mapping_filename);
    column_ = std::stoi(header[1]);
    size_t s = std::stoi(header[2]);
    // The last element is the rightmost bound on the data.
    // Read everything as a double to be safe and then convert to scalar
    
    std::cout << "Reading " << s << " mapped buckets" << std::endl;
    mapped_buckets_.reserve(s); 
    for (size_t i = 0; i < s; i++) {
        auto arr = FileUtils::NextArray<double>(file, 3);
        int32_t mapix = (int32_t)arr[0];
        ScalarRange r = {(Scalar)arr[1], (Scalar)arr[2]};
        mapped_buckets_.emplace(mapix, r);
    }

    header = FileUtils::NextArray<std::string>(file, 2);
    std::cout << header[0] << std::endl;
    AssertWithMessage(header[0] == "mapping", "Bad input file " + mapping_filename);
    s = std::stoi(header[1]);
    AssertWithMessage(s <= mapped_buckets_.size(), "Bad input file " + mapping_filename);
    std::unordered_map<int32_t, std::vector<int32_t>> bucket_mapping;
    bucket_mapping.reserve(s);
    for (size_t i = 0; i < s; i++) {
        auto arr = FileUtils::NextArray<int32_t>(file);
        bucket_mapping.emplace(arr[0], std::vector<int32_t>(arr.begin() + 1, arr.end()));
    } 
    std::cout << "Finished loading mapping-file" << std::endl;
   
    std::ifstream targetfile(target_buckets_filename); 
    AssertWithMessage(targetfile.is_open(), "Couldn't find file: " + target_buckets_filename);
    header = FileUtils::NextArray<std::string>(targetfile, 3);
    AssertWithMessage(header[0] == "target_index_ranges", "bad input file " + target_buckets_filename);
    s = std::stoi(header[2]);
    std::map<int32_t, Range<Key>> targets;
    size_t next_target_bucket = 0;
    for (size_t i = 0; i < s; i++) {
        auto arr = FileUtils::NextArray<size_t>(targetfile, 3);
        AssertWithMessage(arr[0] >= next_target_bucket, "Target buckets not listed in sorted order!");
        next_target_bucket = arr[0] + 1;
        targets.emplace((int32_t)arr[0], Range<Key>(arr[1], arr[2]));
    }
    std::cout << "Finished loading target-buckets-file" << std::endl;

    size_t total_num_ranges = 0;
    for (auto mapit = mapped_buckets_.cbegin(); mapit != mapped_buckets_.cend(); mapit++) {
        ScalarRange mb = mapit->second;
        auto loc = bucket_mapping.find(mapit->first);
        // Can do something else here - we know that if a mapping is present, then there's at least
        // one point that lies in it, even if it's an outlier. If a mapping is not present, we know
        // there's nothing even in the outlier index.
        if (loc == bucket_mapping.end()) {
            // Nothing to be done here.
            continue;
        }
        std::vector<int32_t> tbs = loc->second;
        AssertWithMessage(std::is_sorted(tbs.begin(), tbs.end()), "Ranges not sorted");
        mapping_lst_.insert(std::make_pair(mb, tbs)); 
        Ranges<Key> tixs;
        for (size_t j = 0; j < tbs.size(); j++) {
            auto it = targets.find(tbs[j]);
            // If the key isn't found, there's an inconsistency: it corresponds to an inlier bucket
            // but we missed it when writing out the target buckets.
            AssertWithMessage(it != targets.end(), "Internal error: target bucket not found");
            Range<Key> r = it->second;
            // There must be points contained here, otherwise we shouldn't have written this bucket.
            AssertWithMessage(r.end > r.start, "Internal error: empty bucket");
            if (tixs.size() > 0 && r.start == tixs.back().end) {
                // These ranges are consecutive, so merge them.
                tixs.back().end = r.end;
            } else {
                tixs.push_back(r);
            }
        }
        tixs.shrink_to_fit();
        total_num_ranges += tixs.size();
        mapping_.insert(std::make_pair(mb, tixs));
    }

    target_buckets_.assign(targets.begin(), targets.end());
    std::cout << "Target_buckets_ has size " << target_buckets_.size() << std::endl;
    std::cout << "Finished loading mapped correlation index on " << column_
       << " with " << total_num_ranges << " total ranges and size " << Size() << std::endl;
    size_t list_size = 0;
    for (auto it = mapping_lst_.begin(); it != mapping_lst_.end(); it++) {
        list_size += it->second.size() * sizeof(int32_t);
        list_size += sizeof(ScalarRange);
    }
    // BTree overhead (5 bytes per entry)
    list_size += mapping_lst_.size() * 10;
    std::cout << "List mapping has size: " << list_size << std::endl;
};

template <size_t D>
Ranges<Key> MappedCorrelationIndex<D>::KeyRanges(const Query<D>& q) const {
    size_t col = column_;
    if (!q.filters[col].present) {
        return {{{0, data_size_}}, {}};
    }
    assert (q.filters[col].is_range);
    assert (q.filters[col].ranges.size() == 1);
    ScalarRange sr = q.filters[col].ranges[0];

    // Bucket ends are exclusive, so we want to start at the first bucket whose end is larger than
    // the start of the query range.
    auto startit = mapping_lst_.upper_bound({sr.first, sr.first});
    if (startit == mapping_lst_.end())  {
        return {};
    }
    // Since we're treating query range ends as exclusive, the last range we scan is the one whose
    // end is at least as large as the end of the query range.
    auto endit = mapping_lst_.lower_bound({sr.second, sr.second});
    if (endit != mapping_lst_.end()) {
        endit++;
    }
    std::vector<int32_t> range_ixs;
    for (auto it = startit; it != endit; it++) {
        //range_ixs_tmp.clear();
        //std::set_union(range_ixs.begin(), range_ixs.end(), it->second.begin(), it->second.end(),
        //        std::back_inserter(range_ixs_tmp));
        //range_ixs.swap(range_ixs_tmp);
        range_ixs.insert(range_ixs.end(), it->second.begin(), it->second.end());
    }
    std::sort(range_ixs.begin(), range_ixs.end());
    auto new_end = std::unique(range_ixs.begin(), range_ixs.end());
    range_ixs.erase(new_end, range_ixs.end());
    
    /*std::cout << "Merged ranges (" << ranges.size() << "):" << std::endl;
    for (auto r : ranges) {
        std::cout << "\t" << r.start << " - " << r.end << std::endl;
    }*/
    std::cout << "Mapping query to " << range_ixs.size() << " ranges" << std::endl;
    // Merge with bucket list.

    Ranges<Key> ranges;
    ranges.reserve(range_ixs.size());
    int32_t prev_r = 0;
    auto it = target_buckets_.begin();
    size_t total_size = 0;
    for (int32_t r : range_ixs) {
        while (it->first < r) { it++;}
        total_size += it->second.end - it->second.start;
        if (ranges.size() > 0 && r == prev_r+1) {
            ranges.back().end = (Key)(it->second.end);
        } else {
            ranges.push_back(it->second);
        }
        prev_r = r;
        it++;
    }
    return ranges;

}

template <size_t D>
void MappedCorrelationIndex<D>::AddBucket(int32_t map_bucket, int32_t target_bucket) {
    ScalarRange r = mapped_buckets_[map_bucket];
    mapping_lst_[r].push_back(target_bucket);
}

template <size_t D>
void MappedCorrelationIndex<D>::RemoveBucket(int32_t map_bucket, int32_t target_bucket) {
    auto& tbs = mapping_lst_[mapped_buckets_[map_bucket]];
    auto loc = std::find(tbs.begin(), tbs.end(), target_bucket);
    if (loc != tbs.end()) {
        tbs.erase(loc);
    }
}


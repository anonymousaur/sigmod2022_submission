#pragma once

#include <vector>
#include <map>
#include <string>
#include <unordered_map>

#include "correlation_indexer.h"
#include "types.h"

template <size_t D>
class MappedCorrelationIndex : public Indexer<D> {
  public:
    
    // Starts the index with initialized target buckets.
    MappedCorrelationIndex(const std::string& mapping_file, const std::string& target_bucket_file);

    void Clear() {
        // Remove inlier buckets but preserve mapped and target ranges.
        for (auto it = mapping_.begin(); it != mapping_.end(); it++) {
            it->second.clear();
        }
        for (auto it = mapping_lst_.begin(); it != mapping_lst_.end(); it++) {
            it->second.clear();
        }
    }

    virtual void Init(ConstPointIterator<D> start, ConstPointIterator<D> end)  {
        data_size_ = std::distance(start, end);
        ready_ = true;
    }

    // Return IDs of host buckets that need to be scanned.
    //std::vector<int32_t> HostBucketIds(const Query<D>& q) const ; 
    Ranges<Key> KeyRanges(const Query<D>& q) const;

    size_t Size() const  {
        // Computes size using the list mapping.
        size_t s = mapping_lst_.bytes_used();
        for (auto it = mapping_lst_.begin(); it != mapping_lst_.end(); it++) {
            s += it->second.size() * sizeof(int32_t);
        }
        return s;
    }

    // This is ONLY for use in special cases. Usually the column is automatically set from the
    // mapping file.
    void SetColumn(size_t col) {
        column_ = col;
    }

    size_t GetMappedColumn() {
        return column_;
    }

    ScalarRange GetBucket(int32_t id) {
        return mapped_buckets_[id];
    }

    void AddBucket(int32_t map_bucket, int32_t target_bucket);

    void RemoveBucket(int32_t map_bucket, int32_t target_bucket);

    IndexerType Type() const override {
        return Mapped;
    }

    std::string GetMappingFile() const {
        return mapping_file_;
    }

  private:
    // Load the contents of the files specified in the constructor, used to construct a mapping we
    // can use.
    void Load(const std::string&, const std::string&);
    
    // Number of data points
    size_t data_size_;
    
    // True when the index has been initialized.
    bool ready_;
    size_t column_;    

    // Mapping from mapped dimension bucket value (left bound) to range of indices it corresponds to.
    // TODO: use a CPP B-Tree here (https://code.google.com/archive/p/cpp-btree/) for
    // memory-efficient and fast accesses.
    btree::btree_map<ScalarRange, Ranges<Key>, ScalarRangeComp> mapping_; 
    btree::btree_map<ScalarRange, std::vector<int32_t>, ScalarRangeComp> mapping_lst_;
    std::vector<std::pair<int32_t, Range<Key>>> target_buckets_;
    std::unordered_map<int32_t, ScalarRange> mapped_buckets_;
    std::string mapping_file_;
};

#include "../src/mapped_correlation_index.hpp"

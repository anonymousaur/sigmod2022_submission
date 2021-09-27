#pragma once

#include <vector>
#include <map>
#include <string>
#include <fstream>

#include "secondary_indexer.h"
#include "types.h"

template <size_t D>
class BucketedSecondaryIndex : public SecondaryIndexer<D> {
  public:
    enum BucketingStrategy { UNSET, CONST_WIDTH, FROM_FILE }; 

    // Sets the bucket width to use for bucketing the outliers.
    BucketedSecondaryIndex(size_t dim);
    // Optionally, allows to specify a subset of points to index.
    BucketedSecondaryIndex(size_t dim, List<Key> subset);
    // This is the base constructor.
    BucketedSecondaryIndex(size_t dim, bool use_subset, List<Key> subset);

    void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override;

    List<Key> Matches(const Query<D>& q) const override; 

    void SetBucketFile(const std::string& filename);
    void SetBucketWidth(Scalar width);
    
    size_t Size() const override {
        size_t s = 0;
        for (auto it = buckets_.begin(); it != buckets_.end(); it++) {
            s += it->second.size() * sizeof(Key);
        }
        s += buckets_.bytes_used();
        return s;
    }

    void WriteStats(std::ofstream& statsfile) override {
        statsfile << "num_outliers_col_" << this->column_ << ": " << indexed_size_ << std::endl; 
    }

    void Insert(const std::vector<KeyPair>& inserts) override;
    void Remove(const ScalarRange&, const Range<Key>&) override;


  private:
    // Add to the buckets_ map, but don't order this value with the other existing ones.
    void InsertUnsorted(Key index, Scalar v);
    
    BucketingStrategy bucket_strat_;
    size_t data_size_;
    // The number of points actually indexed.
    size_t indexed_size_;
    // True when the index has been initialized.
    Scalar bucket_width_;
    bool ready_;
    // Whether to use the custom index_subset_
    bool use_subset_;
    List<Key> index_subset_;
    // For each map bucket (key is the start value of the map bucket range), 
    btree::btree_map<Scalar, List<Key>> buckets_;
};

#include "../src/bucketed_secondary_index.hpp"

#pragma once

#include <vector>
#include <memory>

#include "primary_indexer.h"
#include "dataset.h"
#include "types.h"

/*
 * A clustered index on one dimension that uses no auxiliary data structure. The data is sorted and
 * the desired range is found using binary search on the endpoints.
 */
template <size_t D>
class BinarySearchIndex : public PrimaryIndexer<D> {
  public:
    struct HostBucket : public PrimaryIndexNode {
        PhysicalIndex start_index;
        PhysicalIndex end_index;
        Scalar start_value;
        Scalar end_value;
        int32_t bucket_id;
        HostBucket(int32_t id, Scalar start, Scalar end) : bucket_id(id), start_value(start), end_value(end) {}
        int32_t Id() override {
            return bucket_id;
        }
        PhysicalIndex StartOffset() override {
            return start_index;
        }
        PhysicalIndex EndOffset() override {
            return end_index;
        }
        Scalar StartValue() {
            return start_value;
        }
        Scalar EndValue() {
            return end_value;
        }
        std::vector<std::shared_ptr<PrimaryIndexNode>> Descendants() override {
            return {};
        }
    };


    BinarySearchIndex(size_t dim);
    BinarySearchIndex(size_t dim, const std::string& host_bucket_file);

    bool Init(PointIterator<D> start, PointIterator<D> end) override;

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override; 

    void SetDataset(std::shared_ptr<Dataset<D>> dataset) {
        dataset_ = dataset;
    }

    size_t Size() const override {
        return 0;
    }

    void WriteStats(std::ofstream& statsfile) override {
        statsfile << "primary_index_type: rbinary_search_index_" << column_ << std::endl;
    }

    std::vector<InsertRecord<D>> Insert(const std::vector<Point<D>>& new_pts) override;
    std::vector<InsertRecord<D>> DummyInsert(const std::vector<Point<D>>& new_pts, const std::vector<size_t>& indexes) override;

  protected:
    // Returns the index of the first values >= val.
    size_t LocateLeft(Scalar val) const;
    // Returns the index of the first value larger than val.
    size_t LocateRight(Scalar val) const;

    void Load(const std::string& host_bucket_file);

    // Used for internal purposes only.
    size_t column_;
    // True when the index has been initialized.
    bool ready_;

    std::vector<HostBucket> host_buckets_;
    std::shared_ptr<Dataset<D>> dataset_;
};

#include "../src/binary_search_index.hpp"

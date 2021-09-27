#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <fstream>

#include "correlation_indexer.h"
#include "types.h"
#include "mapped_correlation_index.h"
#include "secondary_indexer.h"
#include "correlation_tracker.h"


template <size_t D>
class CombinedCorrelationIndex : public CorrelationIndexer<D> {
  public:
    CombinedCorrelationIndex() : data_size_(0), ready_(false),
        mapped_index_(), outlier_index_(), tracker_(), inlier_buckets_(),
        outlier_buckets_(), outlier_points_() {}

    void SetMappedIndex(std::unique_ptr<MappedCorrelationIndex<D>> mix) {
        mapped_index_ = std::move(mix);
    }

    void SetOutlierIndex(std::unique_ptr<SecondaryIndexer<D>> six) {
       outlier_index_ = std::move(six);
    } 

    void SetTracker(std::unique_ptr<CorrelationTracker<D>> track) {
        tracker_ = std::move(track);
    }

    void SetPrimaryIndex(std::shared_ptr<PrimaryIndexer<D>> pi) {
        primary_index_ = pi;
    }

    void SetDataset(std::shared_ptr<Dataset<D>> dset) override {
        tracker_->SetDataset(dset);
    }

    virtual void InitTracker(ConstPointIterator<D> start, ConstPointIterator<D> end) {
        std::cout << "Initializing tracker" << std::endl;
        tracker_ = std::make_unique<CorrelationTracker<D>>();
        tracker_->SetMappingFile(mapped_index_->GetMappingFile());
        std::cout << "Done initializing tracker" << std::endl;
    }
    /*    std::vector<KeyPair> pts_in_bucket;
        pts_in_bucket.reserve(data_size_ / host_buckets_.size());
        auto hbit = host_buckets_.begin();
        Scalar key = 0;
        for (auto it = start; it != end; it++, key++) {
            while (key > hbit->second->EndOffset()) {
                hbit++;
                if (!pts_in_bucket.empty()) {
                    std::cout << "Inserting into bucket " << hbit->
                    tracker_->InsertBatchList(*hbit, pts_in_bucket);
                    pts_in_bucket.clear();
                }
            }
            pts_in_bucket.push_back(std::make_pair((*it)[this->column_], key));
        }
    }*/

    virtual void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override {
        assert (mapped_index_ && outlier_index_);
        if (outlier_index_) {
            this->column_ = outlier_index_->GetColumn();
            mapped_index_->SetColumn(this->column_);
        } else {
            this->column_ = mapped_index_->GetMappedColumn();
        }
        mapped_index_->Init(start, end);
        if (outlier_index_) {
            outlier_index_->Init(start, end);
        }
        data_size_ = std::distance(start, end);
        ready_ = true;
        InitTracker(start, end); 
        std::cout << "Initialized Combined Correlation Index with column " << this->column_
            << " and size " << Size() << std::endl;
    }
    
    Set<Key> KeyRanges(const Query<D>& q) const;

    size_t Size() const override {
        size_t s = mapped_index_->Size();
        if (outlier_index_) {
            s += outlier_index_->Size();
        }
        //if (tracker_) {
        //    s += tracker_->Size();
        //}
        return s; 
    }

    void WriteStats(std::ofstream& statsfile) const {
        mapped_index_->WriteStats(statsfile);
        if (outlier_index_) {
            outlier_index_->WriteStats(statsfile);
        }
    }

    void Insert(const std::vector<InsertRecord<D>>& records) override;

  private:
    void ProcessAction(int32_t map_bucket, const Action& action);
    void ProcessTrackerDiffs(const DiffMap& diffs);

    // Load the contents of the files specified in the constructor, used to construct a mapping we
    // can use.
    void Load(const std::string&, const std::string&);
    
    // Number of data points
    size_t data_size_;

    //boookkeeping
    size_t inlier_buckets_;
    size_t outlier_buckets_;
    size_t outlier_points_;

    // True when the index has been initialized.
    bool ready_;
    // The combined index consists of two parts: one mapped index which produces only ranges in the
    // target dimension(s), and the outlier map that points to specific indices in the dataset to
    // check in addition to the ranges.
    std::unique_ptr<MappedCorrelationIndex<D>> mapped_index_;
    std::unique_ptr<SecondaryIndexer<D>> outlier_index_;
    std::unique_ptr<CorrelationTracker<D>> tracker_;
    btree::btree_map<int32_t, PrimaryIndexNode *> host_buckets_;
    std::shared_ptr<PrimaryIndexer<D>> primary_index_;
};

#include "../src/combined_correlation_index.hpp"

#pragma once

#include <vector>
#include <random>

#include "primary_indexer.h"
#include "dataset.h"
#include "types.h"
#include "merge_utils.h"

/*
 * A clustered index on one dimension that uses no auxiliary data structure. The data is sorted and
 * the desired range is found using binary search on the endpoints.
 */
template <size_t D>
class MeasureBetaIndex : public PrimaryIndexer<D> {
  public:
    MeasureBetaIndex(size_t dim) {}

    bool Init(PointIterator<D> start, PointIterator<D> end) override {
        //minval_ = std::numeric_limits<Scalar>::max();
        //maxval_ = std::numeric_limits<Scalar>::lowest();
        //for (auto it = start; it != end; it++) {
        //    minval_ = std::min(minval_, (*it)[column_]);
        //    maxval_ = std::max(maxval_, (*it)[column_]);
        //}
        //std::cout << "Maxval = " << maxval_ << std::endl;
        data_size_ = std::distance(start, end);
        //match_size_dist_ = std::uniform_real_distribution<float>(0, log2(0.5*maxval_));
        //range_size_dist_ = std::uniform_real_distribution<float>(0, log2(data_size_));
        //match_start_dist_ = std::uniform_int_distribution<int>(minval_, maxval_);
        //range_start_dist_ = std::uniform_int_distribution<int>(0, data_size_);
        primary_->Init(start, end);
        secondary_->Init(start, end);
        return false;
    }

    void SetDataset(std::shared_ptr<Dataset<D>> dataset) {
        dataset_ = dataset;
    }

    void SetPrimaryIndex(std::unique_ptr<PrimaryIndexer<D>> primary) {
        primary_ = std::move(primary);
    }
    
    void SetAuxiliaryIndex(std::unique_ptr<SecondaryIndexer<D>> secondary) {
        secondary_ = std::move(secondary);
    }

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override {
        auto ranges = primary_->IndexRanges(q);
        size_t range_size = 0;
        size_t nranges = ranges.ranges.size();
        for (auto r : ranges.ranges) {
            range_size += r.end - r.start;
        }
        if (range_size == data_size_) {
            range_size = 0;
            nranges = 0;
        }
        std::cout << "Range size: " << range_size << std::endl;
        std::cout << "Num ranges: " << nranges << std::endl;
        
        size_t list_size = 0;
        Set<PhysicalIndex> aux_matches({}, {});
        if (q.filters[secondary_->GetColumn()].present) {
            auto lookups = secondary_->Matches(q);
            aux_matches = dataset_->Lookup(Set<Key>({}, lookups));
            list_size = aux_matches.list.size();
        }
        Set<PhysicalIndex> merged = MergeUtils::Union<PhysicalIndex>(ranges.ranges, aux_matches.list);
        std::cout << "Match size: " << list_size << std::endl;

        // We don't care about accuracy here.
        // TODO: fetch these records from the dataset.
        return merged;
    } 

    size_t Size() const override {
        return 0;
    }

    void WriteStats(std::ofstream& statsfile) override {
        statsfile << "primary_index_type: measure_beta_index_ with primary_index: ";
        primary_->WriteStats(statsfile);
        statsfile << " and secondary index: ";
        secondary_->WriteStats(statsfile);
        statsfile << std::endl;
    }

  protected:
    std::default_random_engine gen_;
    std::uniform_int_distribution<int> match_start_dist_;
    std::uniform_int_distribution<int> range_start_dist_;
    std::uniform_real_distribution<float> match_size_dist_;
    std::uniform_real_distribution<float> range_size_dist_;
    Scalar minval_;
    Scalar maxval_;
    size_t column_;
    size_t data_size_;
    // True when the index has been initialized.
    bool ready_;
    std::unique_ptr<PrimaryIndexer<D>> primary_;
    std::unique_ptr<SecondaryIndexer<D>> secondary_;
    std::shared_ptr<Dataset<D>> dataset_;
};


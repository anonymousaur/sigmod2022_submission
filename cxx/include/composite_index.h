#pragma once

#include <vector>
#include <memory>
#include <fstream>

#include "types.h"
#include "primary_indexer.h"
#include "secondary_indexer.h"
#include "correlation_indexer.h"
#include "rewriter.h"

/*
 * An index that combines other indexes together. It allows at most one primary index and multiple
 * secondary indexes.
 */
template <size_t D>
class CompositeIndex : public PrimaryIndexer<D> {
  public:
    CompositeIndex(size_t gap_threshold);

    bool SetPrimaryIndex(std::unique_ptr<PrimaryIndexer<D>> primary_index);

    bool AddCorrelationIndex(std::unique_ptr<CorrelationIndexer<D>> corr_index);

    bool AddSecondaryIndex(std::unique_ptr<SecondaryIndexer<D>> index);

    bool AddRewriter(std::unique_ptr<Rewriter<D>> rewriter);

    void SetDataset(std::shared_ptr<Dataset<D>> dset) {
        if (primary_index_) {
            primary_index_->SetDataset(dset);
        }
        for (auto& ci : correlation_indexes_) {
            ci->SetDataset(dset);
        }
        dataset_ = dset;
    }

    virtual bool Init(PointIterator<D> start, PointIterator<D> end) override;

    Set<PhysicalIndex> IndexRanges(Query<D>& q) override; 
    
    InsertRecord<D> Insert(std::vector<Point<D>> new_pts) {
        AssertWithMessage(primary_index_ != NULL, "No primary index to insert into");
        auto idata = primary_index_->Insert(idata);
        for (auto& ci : correlation_indexes_) {
            ci->Insert(idata);
        }
        for (auto& si : secondary_indexes_) {
            si->Insert(idata);
        }
        for (auto& rw : rewriters_) {
            rw->Insert(idata);
        }
    }
    
    size_t Size() const override {
        size_t s = 0;
        if (primary_index_ != NULL) {
            s += primary_index_->Size();
        }
        for (auto& ci : correlation_indexes_) {
            s += ci->Size();
        }
        for (auto& si : secondary_indexes_) {
            s += si->Size();
        }
        for (auto& rw : rewriters_) {
            s += rw->Size();
        }
        return s;
    }

    void WriteStats(std::ofstream& statsfile) override {
        if (primary_index_) {
            primary_index_->WriteStats(statsfile);
        }
        for (auto& ci : correlation_indexes_) {
            ci->WriteStats(statsfile);
        }
        for (auto& si : secondary_indexes_) {
            si->WriteStats(statsfile);
        }
    }

    std::vector<InsertRecord<D>> Insert(const std::vector<Point<D>>& pts) override;
    std::vector<InsertRecord<D>> DummyInsert(const std::vector<Point<D>>& pts,
            const std::vector<size_t>&) override;



  private:
    Set<PhysicalIndex> RangesWithPrimary(Query<D>& q);
    List<Key> RangesWithRewriter(Query<D>& q);
    List<Key> RangesWithSecondary(Query<D>& q);
    Set<Key> RangesWithCorrelation(Query<D>& q);
    
    // If consecutive matching indexes are at or below this gap threshold, includes them in a single
    // range. Otherwise, truncates the old range and starts a new one.
    size_t gap_threshold_;
    // Number of points indexed. 
    size_t data_size_;
    std::shared_ptr<Dataset<D>> dataset_;
    // Once the primary index has been given to the composite index, it should no longer be
    // modified.
    std::unique_ptr<PrimaryIndexer<D>> primary_index_;
    std::vector<std::unique_ptr<SecondaryIndexer<D>>> secondary_indexes_;
    std::vector<std::unique_ptr<CorrelationIndexer<D>>> correlation_indexes_;
    std::vector<std::unique_ptr<Rewriter<D>>> rewriters_;
};

#include "../src/composite_index.hpp"

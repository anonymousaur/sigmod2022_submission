/*
 * The query engine that puts everything together to answer categorial queries, i.e. queries of the
 * type ... WHERE A in (a1, a2, a3) AND ...
 */
#pragma once

#include <vector>
#include <memory>
#include <fstream>

#include "types.h"
#include "indexer.h"
#include "rewriter.h"
#include "dataset.h"
#include "visitor.h"

template <size_t D>
class QueryEngine {
  public:
    QueryEngine(std::shared_ptr<Dataset<D>> dataset,
            std::shared_ptr<PrimaryIndexer<D>> indexer);
    
    void Execute(Query<D>& q, Visitor<D>& visitor);

    long ScannedPoints() const {
        return scanned_range_points_ + scanned_list_points_;
    }
    long ScannedRangePoints() const {
        return scanned_range_points_;
    }
    long ScannedListPoints() const {
        return scanned_list_points_;
    }

    void WriteStats(std::ofstream& statsfile) const {
        indexer_->WriteStats(statsfile);
        statsfile << "avg_scanned_points_in_range: " << scanned_range_points_ / ((float)num_queries_) << std::endl
            << "avg_scanned_points_in_list: " << scanned_list_points_ / ((float)num_queries_) << std::endl
            << "avg_indexing_time_ns: " << indexing_time_ / ((float)num_queries_) << std::endl
            << "avg_range_scan_time_ns: " << range_scan_time_ / ((float)num_queries_) << std::endl
            << "avg_list_scan_time_ns: " << list_scan_time_ / ((float)num_queries_) << std::endl;
    }

    void Reset() {
        scanned_range_points_ = 0;
        scanned_list_points_ = 0;
        range_scan_time_ = 0;
        indexing_time_ = 0;
        list_scan_time_ = 0;
        num_queries_ = 0;
    }

    long IndexerSize() {
        return indexer_->Size();
    }

  private:
    std::shared_ptr<Dataset<D>> dataset_;
    std::shared_ptr<PrimaryIndexer<D>> indexer_;
    // The columns that are indexed by this indexer, saved for faster access.
    std::unordered_set<size_t> columns_;

    long scanned_range_points_;
    long scanned_list_points_; 
    long range_scan_time_;
    long indexing_time_;
    long list_scan_time_;
    size_t num_queries_;
};

#include "../src/query_engine.hpp"


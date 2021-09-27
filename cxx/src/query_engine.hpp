#include "query_engine.h"

#include <vector>
#include <unordered_set>

#include "types.h"
#include "visitor.h"

template <size_t D>
QueryEngine<D>::QueryEngine(
        std::shared_ptr<Dataset<D>> dataset,
        std::shared_ptr<PrimaryIndexer<D>> indexer)
    : dataset_(dataset),
      indexer_(indexer),
      columns_(indexer_->GetColumns()),
      scanned_range_points_(0),
      scanned_list_points_(0),
      range_scan_time_(0),
      indexing_time_(0),
      list_scan_time_(0),
      num_queries_(0) {}

template <size_t D>
void QueryEngine<D>::Execute(Query<D>& q, Visitor<D>& visitor) {
    std::vector<size_t> categorical_query_dimensions;
    std::vector<size_t> range_query_dimensions;
    std::vector<std::unordered_set<Scalar>> value_sets;
    for (size_t i = 0; i < dataset_->NumDims(); i++) {
        if (q.filters[i].present) {
            // If the filtered dimension isn't indexed, this isn't an exact query anymore.
            if (q.filters[i].is_range) {
                assert (q.filters[i].ranges.size() == 1);
                range_query_dimensions.push_back(i);
            } else {
                categorical_query_dimensions.push_back(i);
                value_sets.emplace_back(q.filters[i].values.begin(),
                        q.filters[i].values.end());
            }
        }
    }
    auto preindex = std::chrono::high_resolution_clock::now();
    std::cout << "Starting indexer" << std::endl;
    Set<PhysicalIndex> indexes_to_scan = indexer_->IndexRanges(q);
    auto start = std::chrono::high_resolution_clock::now();
    for (Range<PhysicalIndex> range : indexes_to_scan.ranges) {
        scanned_range_points_ += range.end - range.start;
        for (PhysicalIndex p = range.start; p < range.end; p += 64UL) {
            size_t true_end = std::min(range.end, p + 64UL);
            // A way to get the last true_end - p bits set to 1.
            uint64_t valids = 1ULL + (((1ULL << (true_end - p - 1)) - 1ULL) << 1);
            for (size_t i = 0; i < categorical_query_dimensions.size(); i++) {
                valids &= dataset_->GetCoordInSet(p, true_end,
                        categorical_query_dimensions[i], value_sets[i]);
            }
            for (size_t d : range_query_dimensions) {
                auto r = q.filters[d].ranges[0];
                valids &= dataset_->GetCoordInRange(p, true_end,
                        d, r.first, r.second);
            }
            visitor.visitRange(dataset_.get(), p, true_end, valids); 
        }
    }
    auto mid = std::chrono::high_resolution_clock::now();
    scanned_list_points_ += indexes_to_scan.list.size();
    for (PhysicalIndex p : indexes_to_scan.list) {
        // Can't really do a bitmap for this.
        size_t valid = 1;
        for (size_t i = 0; i < categorical_query_dimensions.size(); i++) {
            valid &= dataset_->GetCoordInSet(p, p+1,
                    categorical_query_dimensions[i], value_sets[i]);
        }
        for (size_t d : range_query_dimensions) {
            auto r = q.filters[d].ranges[0];
            valid &= dataset_->GetCoordInRange(p, p+1,
                    d, r.first, r.second);
        }
        if (valid > 0) {
            visitor.visit(PointRef<D>(dataset_.get(), p));
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ranges_t = std::chrono::duration_cast<std::chrono::nanoseconds>(mid-start).count();
    auto list_t = std::chrono::duration_cast<std::chrono::nanoseconds>(end-mid).count();
    auto index_t = std::chrono::duration_cast<std::chrono::nanoseconds>(start-preindex).count();
    std::cout << "Scan time (us): ranges = " << ranges_t / 1e3
        << ", list = " << list_t / 1e3 << ", total = " << (ranges_t + list_t) / 1e3 << std::endl;
    num_queries_ += 1;
    range_scan_time_ += ranges_t;
    list_scan_time_ += list_t;
    indexing_time_ += index_t;
}


#include "linear_model_rewriter.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "types.h"
#include "file_utils.h"
#include "utils.h"

const std::string HERMIT_OUTLIER_FILEBASE = "hermit_outliers";
const std::string HERMIT_MODEL_FILEBASE = "hermit_model";

// These constants are specified by the Hermit paper, Eval:Implementation section.
template <size_t D>
TRSTreeRewriter<D>::TRSTreeRewriter(size_t mapped, size_t target, float err_bd)
    : mapped_dim_(mapped), target_dim_(target),
      trs_root_(std::make_unique<TRSNode>(err_bd, 0.1, 8, 0, 10)) {
}

template <size_t D>
void TRSTreeRewriter<D>::Init(ConstPointIterator<D> start, ConstPointIterator<D> end) {
    // Get the list of x and y.
    data_size_ = std::distance(start, end);
    std::vector<std::pair<int32_t, std::pair<Scalar, Scalar>>> corr(data_size_);
    for (size_t i = 0; i < data_size_; i++) {
        const Point<D> pt = *(start + i);
        corr[i] = std::make_pair(i, std::make_pair(pt[mapped_dim_], pt[target_dim_]));
    }
    // Sort by mapped dimension.
    std::sort(corr.begin(), corr.end(), [] (const auto& lhs, const auto& rhs) {
            return lhs.second.first < rhs.second.first;
            });

    List<Key> sort_indices(data_size_);
    std::transform(corr.begin(), corr.end(), sort_indices.begin(), [] (const auto& item) {
            return item.first;
            });
    std::vector<Scalar> xs(data_size_);
    std::vector<Scalar> ys(data_size_);
    std::transform(corr.begin(), corr.end(), xs.begin(), [] (const auto& pair) {
            return pair.second.first;
            });
    std::transform(corr.begin(), corr.end(), ys.begin(), [] (const auto& pair) {
            return pair.second.second;
            });
    
    // Normalizes error bound.
    //trs_root_->err_bound = 2. *  sqrt(data_size_ / 4208260.);
    std::cout << "Building TRS Tree from dim " << mapped_dim_ << " to " << target_dim_ << std::endl;
    List<PhysicalIndex> trs_outliers = trs_root_->Build(xs, ys, 0, data_size_);
    std::string filename = HERMIT_MODEL_FILEBASE + "_" + std::to_string(mapped_dim_) + ".bin";
    std::ofstream output(filename);
    trs_root_->Write(output);
    
    List<Key> outliers(trs_outliers.size());
    std::transform(trs_outliers.begin(), trs_outliers.end(), outliers.begin(), [&sort_indices] (const auto& ix) {
            return sort_indices[ix];
            });
    WriteOutliers(outliers);
    auto nodes = trs_root_->Nodes();
    std::cout << "Built TRS Tree with " << nodes.first << " nodes (" << nodes.second
        << " leaves) and size " << trs_root_->Size() << std::endl;
    outlier_index_->SetIndexList(outliers); 
    outlier_index_->Init(start, end);
}

template <size_t D>
void TRSTreeRewriter<D>::WriteOutliers(const List<Key>& outliers) {
    size_t write_size = sizeof(PhysicalIndex) * outliers.size();
    std::string filename = HERMIT_OUTLIER_FILEBASE + "_" + std::to_string(mapped_dim_) + ".bin";
    std::ofstream output(filename);
    AssertWithMessage(output.is_open(), "Couldn't write TRSTree outliers: couldn't open " + filename);
    output.write((char *)outliers.data(), write_size);
    output.flush();
    output.close();
}

template <size_t D>
List<Key> TRSTreeRewriter<D>::Rewrite(Query<D>& q) {
    if (!q.filters[mapped_dim_].present) {
        return {};
    }
    assert (q.filters[mapped_dim_].is_range);
    Scalar start = q.filters[mapped_dim_].ranges[0].first;
    Scalar end = q.filters[mapped_dim_].ranges[0].second;
    
    std::vector<ScalarRange> trs_ranges;
    trs_root_->Lookup(ScalarRange{start, end}, &trs_ranges);
    // Intersect this with the existing ranges in the filter.
    QueryFilter target_qf = q.filters[target_dim_];
    if (target_qf.present) {
        assert (target_qf.is_range && target_qf.ranges.size() == 1);
        std::vector<ScalarRange> existing = {target_qf.ranges[0]};
        trs_ranges = MergeUtils::Intersect(trs_ranges, existing);
    }
    q.filters[target_dim_] = {.present = true, .is_range=true,
        .ranges = trs_ranges};

    if (outlier_index_) {
        return outlier_index_->Matches(q);
    }
    return {};
}


#include "linear_model_rewriter.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

#include "types.h"
#include "file_utils.h"
#include "utils.h"

template <size_t D>
LinearModelRewriter<D>::LinearModelRewriter(const std::string& filename)
    : outlier_index_() {
    Load(filename);
}

template <size_t D>
void LinearModelRewriter<D>::Load(const std::string& filename) {
    std::ifstream file(filename);
    AssertWithMessage(file.is_open(), "Couldn't find file " + filename);
    AssertWithMessage(FileUtils::NextLine(file) == "linear", "Bad input " + filename);

    auto dims = FileUtils::NextArray<size_t>(file, 2);
    mapped_dim_ = dims[0];
    target_dim_ = dims[1];
    auto coeffs = FileUtils::NextArray<double>(file, 2);
    linear_coeffs_.first = coeffs[0];
    linear_coeffs_.second = coeffs[1];
    auto offset = FileUtils::NextArray<double>(file, 1);
    model_offset_ = offset[0];
    std::cout << "Loaded linear model " << mapped_dim_ << " -> " << target_dim_
        << ", coeffs: " << linear_coeffs_.first << " " << linear_coeffs_.second << ", offset = " << model_offset_ << std::endl;
}
    
template <size_t D>
void LinearModelRewriter<D>::Init(ConstPointIterator<D> start, ConstPointIterator<D> end) {
    List<Key> outliers;
    size_t ix = 0;
    for (auto it = start; it != end; it++, ix++) {
        const Point<D>& p = *it;
        ScalarRange inlier_range = RangeFor(p[mapped_dim_]);
        if (p[target_dim_] < inlier_range.first || p[target_dim_] > inlier_range.second) {
           outliers.push_back(ix);
        } 
    }
    outlier_index_->SetIndexList(outliers); 
    outlier_index_->Init(start, end);
}

template <size_t D>
ScalarRange LinearModelRewriter<D>::RangeFor(Scalar val) {
    double offset = linear_coeffs_.second > 0 ? model_offset_ : -model_offset_;
    double tstart = linear_coeffs_.first + linear_coeffs_.second * val - offset;
    double tend = linear_coeffs_.first + linear_coeffs_.second * val + offset;
    if (tend < tstart) {
        std::swap(tstart, tend);
    }
    // This range is inclusive.
    return {(Scalar)ceil(tstart), (Scalar)tend};
}

template <size_t D>
List<Key> LinearModelRewriter<D>::Rewrite(Query<D>& q) {
    if (!q.filters[mapped_dim_].present) {
        return {};
    }
    assert (q.filters[mapped_dim_].is_range);
    Scalar start = q.filters[mapped_dim_].ranges[0].first;
    Scalar end = q.filters[mapped_dim_].ranges[0].second;   

    ScalarRange start_range = RangeFor(start);
    ScalarRange end_range = RangeFor(end);
    ScalarRange combined = {min(start_range.first, end_range.first),
        max(start_range.second, end_range.second)};
    
    // Intersect this with the existing ranges in the filter.
    QueryFilter target_qf = q.filters[target_dim_];
    if (target_qf.present) {
        assert (target_qf.is_range);
        ScalarRange existing = target_qf.ranges[0];
        combined.first = max(combined.first, existing.first);
        combined.second = min(combined.second, existing.second);
    }
    
    std::vector<ScalarRange> ranges;
    ranges.push_back(combined);
    
    q.filters[target_dim_] = {.present=true, .is_range=true, .ranges=ranges};
    if (outlier_index_) {
        return outlier_index_->Matches(q);
    }
    return {};
}


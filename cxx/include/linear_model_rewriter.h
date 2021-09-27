/*
 * Dispatches to multiple linear rewriters on different columns.
 */

#pragma once

#include <string>
#include <vector>

#include "types.h"
#include "rewriter.h"
#include "secondary_indexer.h"

template <size_t D>
class LinearModelRewriter : public Rewriter<D> {
  public:
    // Loads the one-to-one rewriter from a file.
    LinearModelRewriter(const std::string& filename);

    void SetAuxiliaryIndex(std::unique_ptr<SecondaryBTreeIndex<D>> index) {
        outlier_index_ = std::move(index);
    }

    void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override;

    List<Key> Rewrite(Query<D>& q) override;

    size_t Size() const override {
        size_t s = 3*sizeof(double) + 2*sizeof(size_t);
        if (outlier_index_ != nullptr) {
            s += outlier_index_->Size();
        }
        return s;
    }    

  protected:
    void Load(const std::string& filename);

    // Return the target range for this value. Range bounds are inclusive.
    ScalarRange RangeFor(Scalar val);

  private:
    // The width around the line that's considered inlier territory. Everything outside of this
    // offset is an outlier.
    // For inliers:
    // target_dim is within linear_coeffs.[1, mapped_dim] +/- model_offset
    double model_offset_;
    // Coefficients of the linear map. The ith coefficient corresponds to the ith-degree term.
    std::pair<double, double> linear_coeffs_;
    // The rewriter maps values in mapped_dim_ to values in target_dim_ and adds those target_dim_
    // values to the query.
    size_t mapped_dim_;
    size_t target_dim_;
    std::unique_ptr<SecondaryBTreeIndex<D>> outlier_index_;
};

#include "../src/linear_model_rewriter.hpp"

/**
 * A row-ordered dataset is one where all the fields of each point are stored
 * contiguously.
 */

#include "dataset.h"
#include "types.h"
#include <vector>

#pragma once

template <size_t D>
class RowOrderDataset : public Dataset<D> {
  public:
    explicit RowOrderDataset(std::vector<Point<D>> data) {
        // Makes a copy.
        data_ = data;
    }

    Point<D> Get(size_t id) const override {
        return data_[id];
    }

    Scalar GetCoord(size_t id, size_t dim) const override {
        return data_[id][dim];
    }

    size_t Size() const override {
        return data_.size();
    }

    size_t NumDims() const override {
        return D;
    }

    size_t SizeInBytes() const override {
        return data_.size() * sizeof(Point<D>);
    }

  private:
    std::vector<Point<D>> data_;
};


/**
 * A column-ordered dataset, which stores a single coordinate for each point continguously.
 * Each coordinate is stored in a separate data structure.
 */

#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <cassert>

#include "dataset.h"
#include "datacube.h"
#include "types.h"

template <size_t D>
class InMemoryColumnOrderDataset : public Dataset<D> {
  
  public:
    explicit InMemoryColumnOrderDataset(std::vector<Point<D>> data) {
        // Index the datacubes by the columns they will appear in.
        for (size_t d = 0; d < D; d++) {
            columns_[d].reserve(data.size());
        }
        for (const Point<D> &p : data) {
            for (size_t d = 0; d < D; d++) {
                columns_[d].push_back(p[d]);
            }
        }
    }

    Point<D> Get(size_t i) const override {
        Point<D> data;
        for (size_t d = 0; d < D; d++) {
            data[d] = GetCoord(i, d);
        }
        return data;
    }

    /**
     * Get coordinate `dim` of point at index `index`
     */
    Scalar GetCoord(size_t index, size_t dim) const override {
        return columns_[dim][index];
    }

    size_t Size() const override {
        return columns_[0].size();
    }

    size_t NumDims() const override {
        return D;
    }

    size_t SizeInBytes() const override {
        return Size() * NumDims() * sizeof(Scalar);
    }

    void Insert(InsertData<D>& locations) override {
        std::sort(locations.begin(), locations.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.second < rhs.second;
                });

        std::array<std::vector<Scalar>, D> new_columns;
        for (size_t i = 0; i < D; i++) {
            new_columns[i].reserve(Size() + locations.size());
        }
        size_t data_ix = 0;
        size_t inserted = 0;
        for (auto& loc : locations) {
            for (size_t d = 0; d < D; d++) {
                new_columns[d].insert(new_columns[d].end(), columns_[d].begin() + data_ix,
                        columns_[d].begin() + loc.second);
            }
            data_ix = loc.second;
            for (size_t d = 0; d < D; d++) {
                new_columns[d].push_back(loc.first[d]);
            }
            loc.second += inserted;
            inserted++;
        }
        std::swap(columns_, new_columns);
    }

private:
    std::array<std::vector<Scalar>, D> columns_;
    
};


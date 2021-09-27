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

#include "primary_btree_index.h"
#include "dataset.h"
#include "datacube.h"
#include "types.h"

template <size_t D>
class ClusteredColumnOrderDataset : public Dataset<D> {
  
  public:
    explicit ClusteredColumnOrderDataset(std::vector<Point<D>> data) {
        // Index the datacubes by the columns they will appear in.
        for (size_t d = 0; d < D+1; d++) {
            columns_[d].reserve(data.size());
        }
        PhysicalIndex ix = 0;
        Scalar key = 0;
        for (const Point<D> &p : data) {
            for (size_t d = 0; d < D; d++) {
                columns_[d].push_back(p[d]);
            }
            columns_[D].push_back(key);
            clustered_index_.insert(std::make_pair(key, ix));
            key++;
            ix++;
        }
        // Insert one above the max key at the end, for bookkeeping.
        clustered_index_.insert(std::make_pair(key, ix));
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

    Range<PhysicalIndex> LookupRange(Range<Key> range) {
        auto startit = clustered_index_.lower_bound(range.start);
        auto endit = clustered_index_.lower_bound(range.end);
        return Range<PhysicalIndex>(startit->second, endit->second);
    }

    Set<PhysicalIndex> Lookup(Set<Key> keys) override {
        Set<PhysicalIndex> results;
        results.ranges.reserve(keys.ranges.size());
        results.list.reserve(keys.list.size());
        for (const auto& r : keys.ranges) {
            results.ranges.push_back(LookupRange(r));
        }
        for (const auto& r : keys.list) {
            auto loc = clustered_index_.find(r);
            AssertWithMessage(loc != clustered_index_.end(), "Search for invalid key");
            results.list.push_back(loc->second);
        }
        return results;
    }


    void Insert(InsertData<D>& locations) override {
        std::sort(locations.begin(), locations.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.second < rhs.second;
                });

        std::array<std::vector<Scalar>, D+1> new_columns;
        for (size_t i = 0; i < D+1; i++) {
            new_columns[i].reserve(Size() + locations.size());
        }
        size_t data_ix = 0;
        size_t inserted = 0;
        for (auto& loc : locations) {
            for (size_t d = 0; d < D+1; d++) {
                new_columns[d].insert(new_columns[d].end(), columns_[d].begin() + data_ix,
                        columns_[d].begin() + loc.second);
            }
            // TODO(vikram): This is WRONG!
            new_columns[D].push_back(0);
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
    std::array<std::vector<Scalar>, D+1> columns_;
    btree::btree_map<Key, PhysicalIndex> clustered_index_;
};


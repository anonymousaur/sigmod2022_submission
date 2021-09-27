/**
 * An abstraction for a dataset that allows access to points at a particular index.
 */

#include <unordered_set>

#include "types.h"

#pragma once

template <size_t D>
class Dataset { 
  public:
    // Get all coordinates of a particular data point.
    virtual Point<D> Get(size_t id) const = 0;
    // Get a single coordinate (in dimension `dim`) of the data point at location `id`.
    virtual Scalar GetCoord(size_t id, size_t dim) const = 0;

    // Looks up the indexes for the corresponding primary keys.
    virtual Set<PhysicalIndex> Lookup(Set<Key> keys) {
        Ranges<PhysicalIndex> ranges;
        List<PhysicalIndex> list;
        for (const auto& k : keys.ranges) {
            ranges.emplace_back((PhysicalIndex)(k.start), (PhysicalIndex)(k.end));
        }
        for (const auto& k : keys.list) {
            list.push_back((PhysicalIndex)k);
        }
        return Set<PhysicalIndex>(ranges, list);
    };

    // Return a bitstring denoting all the indices between start (inclusive) and end (exclusive),
    // whose coordinate at dimension `dim` is in the given set of values `vset`.
    // Precondition: end - start <= 64. If end - start < 64, the remaining most significant bits should be 0.
    virtual uint64_t GetCoordInSet(size_t start, size_t end, size_t dim, const std::unordered_set<Scalar>& vset) const {
        uint64_t valid = 0;
        for (size_t i = start; i < end; i++) {
            valid <<= 1;
            Scalar c = GetCoord(i, dim);
            valid |= (vset.find(c) != vset.end());
        }
        return valid;
    }
    
    // Return a bitstring denoting all the indices between start (inclusive) and end (exclusive),
    // whose coordinate at dimension `dim` falls between low (inclusive) and high (exclusive)
    // Precondition: end - start <= 64. If end - start < 64, the remaining most significant bits should be 0.
    virtual uint64_t GetCoordInRange(size_t start, size_t end, size_t dim, Scalar low, Scalar high) const {
        uint64_t valid = 0;
        for (size_t i = start; i < end; i++) {
            valid <<= 1;
            Scalar c = GetCoord(i, dim);
            valid |= c >= low && c < high;
        }
        return valid;
    }
    
    // Return a bitstring denoting all the indices between start (inclusive) and end (exclusive),
    // whose coordinate at dimension `dim` falls between `lower` and `upper` (inclusive).
    // Precondition: end - start <= 64. If end - start < 64, the remaining most significant bits should be 0.
    virtual uint64_t GetCoordRange(size_t start, size_t end, size_t dim, Scalar lower, Scalar upper) const {
        uint64_t valid = 0;
        for (size_t i = start; i < end; i++) {
            valid <<= 1;
            Scalar c = GetCoord(i, dim);
            valid |= (c >= lower && c <= upper);
        }
        return valid;
    }
    
    virtual void GetRangeValues(size_t start, size_t end, size_t dim, uint64_t valids, std::vector<Scalar> *results) const {
        uint64_t mask = 1UL << (end - start - 1);
        for (size_t i = start; i < end; i++) {
            if (valids & mask) {
                results->push_back(GetCoord(i, dim));
            }
            mask >>= 1;
        }
    }
    
    virtual Scalar GetRangeSum(size_t start, size_t end, size_t dim, uint64_t valids) const {
        uint64_t mask = 1UL << (end - start - 1);
        Scalar sum = 0;
        for (size_t i = start; i < end; i++) {
            if (valids & mask) {
                sum += GetCoord(i, dim);
            }
            mask >>= 1;
        }
        return sum;
    }
    
    virtual size_t Size() const = 0;
    virtual size_t NumDims() const = 0;
    // size of the dataset in bytes
    virtual size_t SizeInBytes() const = 0;

    virtual void Insert(InsertData<D>& raw_locs) {};
};

/*
 * A reference to a point, specified with a database pointer and
 * an index. The individual coordinates can be accessed using:
 *   dataset->GetCoord(idx, <dim>)
 */
template <size_t D>
struct PointRef {
    const Dataset<D>* dataset;
    size_t idx;
    PointRef(const Dataset<D>* d, size_t i) {
        dataset = d;
        idx = i;
    }
};


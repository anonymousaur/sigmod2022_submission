/**
 * A visitor is a generic interface to act on results from a range query.
 * Instead of copying all points within a range, a query is issued with a
 * visitor, which dictates how the results are processed. For example, a visitor
 * may store a vector internally to return all the points satisfied by a query,
 * or may simply increment a counter to implement a typical COUNT(*) query.
 */

#pragma once

#include <atomic>

#include "types.h"
#include "dataset.h"

/**
 * A standard visitor pattern, where visit() is called on each result.
 */
template <size_t D>
class Visitor {
  protected:
    long scanned_points_ = 0;
  public:

    long ScannedPoints() { return scanned_points_; }
    // Aggregate the given point as part of the result set.
    virtual void visit(const PointRef<D>& p) = 0;
    // A way to batch process many results. `valids` tells us which indices between `start_ix`
    // (inclusive) and `end_ix` (exclusive) are valid results and should be aggregated.
    virtual void visitRange(const Dataset<D>* dataset, size_t start_ix, size_t end_ix, uint64_t valids) {
       uint64_t mask = 1UL << (end_ix - start_ix - 1);
       scanned_points_ += end_ix - start_ix;
       for (size_t i = start_ix; i < end_ix; i++) {
           if (valids & mask) {
               visit(PointRef<D>(dataset, i));
           }
           mask >>= 1;
       }
    }
    // All the points in the given range are valid results and should be aggregated.
    // This functionality is exposed because it allows for aggregation-specific optimizations.
    virtual void visitExactRange(const Dataset<D>* dataset, size_t start_ix, size_t end_ix) {
       scanned_points_ += end_ix - start_ix;
       for (size_t i = start_ix; i < end_ix; i++) {
           visit(PointRef<D>(dataset, i));
       }
    };
};

/**
 * A specialized visitor with special logic to handle ranges.
 * If the index knows that all points between physical indices s and t satisfy
 * the query, it will call:
 *   visitRange(data, s, t)
 * on the visitor. Otherwise, it will call visit() on each matching point, as
 * with a standard Visitor.
 */
//class RangeVisitor : Visitor {
//   public:
//    // If true is returned iteration should be stopped
//    virtual bool visit(const DataVector& data, PhysicalIndex start,
//                            PhysicalIndex end) = 0;
//};

template <size_t D>
class DummyVisitor: public Visitor<D> {
public:
    DummyVisitor<D>() = default;

    void visit(const PointRef<D>&) override {

    }

    void visitRange(const Dataset<D> *, size_t, size_t, uint64_t) override {

    }

    void visitExactRange(const Dataset<D> *, size_t, size_t) override {

    }
};

template <size_t D>
class CountVisitor: public Visitor<D> {
public:
    size_t count = 0;
    CountVisitor<D>() : count(0) {}

    void visit(const PointRef<D>&) override {
        count += 1;
    }

    void visitRange(const Dataset<D>*, size_t start, size_t end, uint64_t valids) override {
        count += __builtin_popcountll(valids);
        this->scanned_points_ += end - start;
    }

    void visitExactRange(const Dataset<D>*, size_t start, size_t end) override {
        count += end - start;
        this->scanned_points_ += count;
    }
};

template <size_t D>
class CollectVisitor: public Visitor<D> {
public:
    size_t count = 0;
    std::vector<Point<D>> result_set;

    CollectVisitor(): result_set() {};

    void visit(const PointRef<D>& p) override {
        result_set.push_back(p.dataset->Get(p.idx));
    }
};

// Gather the indices of all the resulting points.
template <size_t D>
class IndexVisitor : public Visitor<D> {
  public:
    std::vector<size_t> indexes;
    IndexVisitor() {}

    void visit(const PointRef<D>& p) override {
        indexes.push_back(p.idx);
    }
};

template <size_t D>
class SumVisitor: public Visitor<D> {
public:
    Scalar sum;
    size_t column;
    SumVisitor(size_t col) : sum(0), column(col) {}

    void visit(const PointRef<D>& p) override {
        sum += p.dataset->GetCoord(p.idx, column);
    }
};

template <size_t D>
class SumProductVisitor: public Visitor<D> {
public:
    Scalar sum;
    size_t column1, column2;
    SumProductVisitor(size_t col1, size_t col2) : sum(0), column1(col1), column2(col2) {}

    void visit(const PointRef<D>& p) override {
        sum += p.dataset->GetCoord(p.idx, column1) * p.dataset->GetCoord(p.idx, column2);
    }
};

// Hard-coded for TPC-H Q6 for now
template <size_t D>
class AggregateVisitor: public Visitor<D> {
public:
    Scalar aggregate;
    AggregateVisitor() : aggregate(0) {}

    void visit(const PointRef<D>& p) override {
        aggregate += p.dataset->GetCoord(p.idx, 3) * p.dataset->GetCoord(p.idx, 6);
    }
};

template <size_t D>
class AggregateWithCubesVisitor : public AggregateVisitor<D> {

  public:
    AggregateWithCubesVisitor(size_t sum_cube_dim, size_t cumsum_cube_dim)
        : AggregateVisitor<D>(), sum_cube_dim_(sum_cube_dim), cumsum_cube_dim_(cumsum_cube_dim) {}

    /*void visitRange(const Dataset<D>* dset, size_t start, size_t end, uint64_t valids) {
        this->aggregate.fetch_add(dset->GetRangeSum(start, end, sum_cube_dim_, valids), std::memory_order_relaxed);
    }

    void visitExactRange(const Dataset<D>* dset, size_t start, size_t end) {
        // Access datacube in the last dimension.
        Scalar delta = 0;
        for (size_t i = start; i < end; i++) {
            delta += dset->GetCoord(i, sum_cube_dim_);
        }
        //Scalar delta = dset->GetCoord(end, cumsum_cube_dim_) - dset->GetCoord(start, cumsum_cube_dim_);
        this->aggregate.fetch_add(delta, std::memory_order_relaxed);
    }*/
  
  private:
    const size_t sum_cube_dim_;
    const size_t cumsum_cube_dim_;
};

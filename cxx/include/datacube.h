/**
 * Definition of a datacube, an extra column in a dataset that's used to speed up aggregations.
 * Datacubes can speed up the code in two ways:
 *   - If an aggregation involves two columns in a non-separable manner, i.e. SUM(p[5] * p[6]), then
 *     precomputing dc = p[5] * p[6] allows us to aggregate SUM(dc) at query time. This avoids the
 *     cost of fetching from multiple columns and can be more efficiently block decoded.
 *   - When using the learned index, there may be Virtual index ranges that are "exact", i.e. all
 *   points in the range are known to match the query filter predicates. In these cases, keeping a
 *   datacube that encodes the running sum of points along the sort order allows us to obtain the
 *   sum over an entire range (start, end) by just computing dc(end) - dc(start).
 *
 * Datacubes need to be known in two places:
 *  - When constructing the dataset, the datacubes must also be constructed and compressed alongside
 *    the original data.
 *  - The visitor should be aware of which datacube it can harness.
 *
 * Note that the indexer, locator, and index itself do not need to know anything about the datacube,
 * since it's abstracted away as part of the dataset.
 */

#include "types.h"

#pragma once

// Used to specify what type of aggregation the 
template <size_t D>
struct Aggregator {
    // Produce the next value for this datacube from the current point and the previous datacube
    // value.
    virtual Scalar operator() (const Point<D>& pt) = 0;
};

template <size_t D>
struct Datacube {
    // Every Datacube has a unique index representing the column it occupies in the dataset.
    size_t index;
    Aggregator<D> *agg;
};

// TODO(vikram): These should be declared by
template <size_t D>
struct SumAggregator : public Aggregator<D> {
    Scalar operator() (const Point<D>& pt) override {
        return pt[3]*pt[6];
    }
};

template <size_t D>
struct CumulSumAggregator : public Aggregator<D> {
    Scalar prev = 0;
    Scalar operator() (const Point<D>& pt) override {
        // We have to displace by 1 so that dc[i] has the cumulative sum of every point
        // up to but NOT including i.
        Scalar ret = prev;
        prev += pt[3]*pt[6];
        return ret;
    }
};


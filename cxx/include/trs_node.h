#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

#include "types.h"
#include "math_utils.h"
#include "merge_utils.h"

const Scalar NINF = -(1LL << 45);

struct TRSNode {
    std::vector<std::unique_ptr<TRSNode>> children;
    // The bounds on the mapped column this node is responsible for.
    ScalarRange bounds; 
    // tolerance is the same as \epsilon from the paper
    double slope, intercept, err_bound, outlier_ratio, tolerance;
    int fanout, depth, max_depth;

    TRSNode(float err_bnd, float outlier_frac, int nchild, int d, int max_d) 
        : children(), slope(0), intercept(0), err_bound(err_bnd),
            outlier_ratio(outlier_frac), fanout(nchild), depth(d), max_depth(max_d) {
    }

    std::vector<size_t> Split(const std::vector<Scalar>& xs, const std::vector<Scalar>& ys, size_t index_start, size_t index_end) { 
        Scalar xmin = xs[index_start];
        Scalar xmax = xs[index_end-1];
        double div = (xmax - xmin + 1) / (double)fanout;
        double thresh = div + xmin;
        size_t prev_ix = index_start;
        children.reserve(fanout);
        std::vector<size_t> outliers;
        for (size_t ix = index_start; ix < index_end; ix++) {
           if (xs[ix] >= thresh) {
                while (thresh <= xs[ix]) {
                    thresh += div;
                }
                std::unique_ptr<TRSNode> child = std::make_unique<TRSNode>(
                        err_bound, outlier_ratio, fanout, depth+1, max_depth);
                children.push_back(std::move(child));
                auto out = children.back()->Build(xs, ys, prev_ix, ix);
                outliers.insert(outliers.end(), out.begin(), out.end());
                prev_ix = ix;
           }
        }
        AssertWithMessage(children.size() < fanout, "Got " + std::to_string(children.size()) + 
                " children but was expecting less than " + std::to_string(fanout) + ": " +
                " xmin = " + std::to_string(xmin) + ", xmax = " + std::to_string(xmax) + 
                ", div = " + std::to_string(div));
        std::unique_ptr<TRSNode> child = std::make_unique<TRSNode>(
                err_bound, outlier_ratio, fanout, depth+1, max_depth);
        children.push_back(std::move(child));
        auto out = children.back()->Build(xs, ys, prev_ix, index_end);
        outliers.insert(outliers.end(), out.begin(), out.end());
        return outliers; 
    }

    // x values must be sorted. index_end is exclusive.
    std::vector<size_t> Build(const std::vector<Scalar>& xs, const std::vector<Scalar>& ys,
            size_t index_start, size_t index_end) {
        std::vector<size_t> outlier_indexes;
        if (depth == 0) {
            std::cout << "Building TRS Tree with err_bound = " << err_bound
                << ", max_depth = " << max_depth
                << ", outlier_ratio = " << outlier_ratio
                << ", fanout = " << fanout << std::endl;
            AssertWithMessage(std::is_sorted(xs.begin(), xs.end()), "X values are not sorted");
            while(xs[index_start] < NINF) {
                outlier_indexes.push_back(index_start);
                index_start++;
            }
            std::cout << "Found " << index_start << " NULL values" << std::endl;
        }
        Scalar xmin = xs[index_start];
        Scalar xmax = xs[index_end-1];
        size_t npts = index_end - index_start;
        // Many points with the same value
        bounds = {xmin, xmax};
        if (xmin == xmax) {
            // Take all y values within 1 stdev of the mean.
            double yavg = std::accumulate(ys.begin() + index_start, ys.begin() + index_end, 0.0);
            yavg /= npts;
            double y2 = std::inner_product(ys.begin() + index_start, ys.begin() + index_end,
                    ys.begin() + index_start, 0.0);
            y2 /= npts;
            tolerance = err_bound/2.; //sqrt(y2 - yavg*yavg);
            intercept = 0;
            slope = double(yavg) / xmin;
        } else {
            auto coeffs = LinearModel::Fit(xs.begin() + index_start, xs.begin() + index_end, ys.begin() + index_start);
            intercept = coeffs.second;
            slope = coeffs.first;
            tolerance = fabs(slope * (xmax - xmin) * err_bound / (2.*npts));
            size_t num_outliers = 0;
            for (size_t i = index_start; i < index_end; i++) {
                double line = slope* xs[i] + intercept;
                num_outliers += (line + tolerance < ys[i] || line-tolerance > ys[i]); 
            }
            if (num_outliers > npts * outlier_ratio && depth < max_depth) {
                std::cout << "Splitting node: depth = " << depth << ", slope = " << slope
                    << ", intercept = " << intercept
                    << ", tolerance = " << tolerance
                    << ", num_outliers = " << num_outliers << std::endl;
                auto outliers = Split(xs, ys, index_start, index_end);
                if (outlier_indexes.size() > 0) {
                    outliers.insert(outliers.end(), outlier_indexes.begin(), outlier_indexes.end());
                }
                return outliers;
            }
        }
        outlier_indexes.reserve(outlier_ratio * xs.size() + outlier_indexes.size());
        for (size_t i = index_start; i < index_end; i++) {
            double line = slope * xs[i] + intercept;
            if (line + tolerance < ys[i] || line-tolerance > ys[i]) {
               outlier_indexes.push_back(i);
            } 
        }
        return outlier_indexes;
    }

    void Lookup(ScalarRange query_range, std::vector<ScalarRange>* ranges) {
        if (query_range.first > bounds.second || query_range.second < bounds.first) {
            // Nothing to do here.
            return;
        }
        if (children.size() > 0) {
            for (auto& c : children) {
                c->Lookup(query_range, ranges);
            }
        } else {
            Scalar min_x = std::max(query_range.first, bounds.first);
            Scalar max_x = std::min(query_range.second, bounds.second);
            if (slope < 0) {
                ranges->emplace_back(slope * max_x + intercept - tolerance,
                        slope * min_x + intercept + tolerance+1);
            } else {
                ranges->emplace_back(slope * min_x + intercept - tolerance, slope * max_x + intercept + tolerance+1);
            }
        }
        if (depth > 0) {
            return;
        }
        // Otherwise, this is the root and we need to sort and merge them.
        std::sort(ranges->begin(), ranges->end(), ScalarRangeStartComp{});
        std::cout << "Merge ranges begin" << std::endl;
        auto merged = MergeUtils::Coalesce(ranges->begin(), ranges->end());
        std::cout << "Merge ranges end" << std::endl;
        ranges->assign(merged.begin(), merged.end());
    }

    size_t Size() {
        size_t s = sizeof(ScalarRange); // bounds
        if (children.size() > 0) {
            s += sizeof(std::vector<std::unique_ptr<TRSNode>>);
            for (auto& c : children) {
                s += 8; // 8 bytes for a pointer to the child
                s += c->Size();
            }
        } else {
            s += 3 * sizeof(double); // slope, intercept and tolerance.
        }
        return s;
    }

    // Return a pair of <# total nodes, # leaves>
    std::pair<size_t, size_t> Nodes() {
        if (children.empty()) {
            return std::make_pair(1, 1);
        } else {
            size_t total = 0;
            size_t leaves = 0;
            for (auto& c : children) {
                auto nds = c->Nodes();
                total += nds.first;
                leaves += nds.second;
            }
            total += 1;
            return std::make_pair(total, leaves);
        }
    }

    void Write(std::ofstream& out) {
        if (children.size() > 0) {
            for (auto& c : children) {
                c->Write(out);
            }
        } else {
            out << bounds.first << "\t" << bounds.second << "\t:\t"
                << slope << "\t" << intercept << "\t" << tolerance << std::endl;
        } 
    } 

};

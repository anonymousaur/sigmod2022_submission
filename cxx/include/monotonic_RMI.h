#pragma once

#include "types.h"

#include <array>
#include <fstream>
#include <vector>

const int DEFAULT_LAST_LAYER_EXPERTS = 50;

// A single binary node that fits in a single cache line and forms the
// last layer of the RMI. This helps fix problems where the data shape
// makes a single segment fit poorly.
struct Split {
    Segment first_seg;
    Segment second_seg;
    Scalar split;
    Split() = default;
};

// Keeps track of the state to compute the RMS of an input stream.
struct RMS {
    ldouble x = 0, x2 = 0, y = 0, y2 = 0, xy = 0;
    size_t size = 0;
    RMS() = default;
    /*RMS(ldouble x1, ldouble x2, ldouble x3, ldouble x4, ldouble x5, size_t n) {
        x = x1;
        x2 = x2;
        y = x3;
        y2 = x4;
        xy = x5;
        size = n;
    }*/
    void Update(Scalar xx, size_t yy) {
        x += xx;
        x2 += xx*xx;
        y += yy;
        y2 += yy*yy;
        xy += xx*yy;
        size++;
    }

    RMS Sub(const RMS& other) const {
        return {x - other.x,
                x2 - other.x2,
                y - other.y,
                y2 - other.y2,
                xy - other.xy,
                size - other.size};
    }

    double GetRms() const {
        ldouble s = size;
        ldouble dx = x / s;
        ldouble dy = y / s;
        ldouble sxy = xy - dx*dy*s;
        ldouble sxx = x2 - dx*dx*s;
        ldouble syy = y2 - dy*dy*s;
        return syy - sxy * sxy / sxx;
    }
};

/**
 * The purpose of this class is to represent a model able to
 * convert a virtual disk location (between 0 and 1, non uniform)
 * and map it to the real position on disk as close as possible
 *
 * The implementation uses tree of Linear models
 *
 * @tparam depth the depth of the tree used
 */
template<size_t depth>
class MonotonicRMI {

public:

    MonotonicRMI();
    explicit MonotonicRMI(std::string filename);
    explicit MonotonicRMI(std::array<std::vector<Segment>, depth> parameters);
    explicit MonotonicRMI(const std::vector<Scalar>& data, std::vector<size_t> layer_experts = {DEFAULT_LAST_LAYER_EXPERTS});
    //MonotonicRMI(const MonotonicRMI<depth>& other);

    // Given an x value, return the CDF. The return value will be between 0 and 1. Compile with
    // -DRMI_DEBUG for debugging statements during each RMI evaluation.
    double At(Scalar value) const;
    // Given a CDF value (percentile, between 0 and 1), find the corresponding x-value.
    double Inverse(double yval) const;
    void SetMinMaxVals(Scalar min_val, Scalar max_val);
    void SetDataSize(size_t datasize);
    size_t SizeInBytes() const;
    // The average / max number of indices the RMI is off by (evaluated on the training set).
    double AvgError() const;
    double MaxError() const;

    // Methods used to modify the RMI.
    Segment GetLastLayerSegment(size_t segment_ix);
    void SetLastLayerSegment(size_t segment_ix, const Segment& seg);
    std::vector<int64_t> GetSeparators() const;
    // Given all the values in the dataset for this dimension, compute the separators from scratch.
    // Getting them from Python is inaccurate.
    std::vector<Scalar> ComputeSeparators(const std::vector<Scalar>& values) const;
    size_t NumLastLayerSegments() const;
    // Train the RMI for a particular set of values. Public in case the RMI is instantiated without
    // arguments.
    // Returns true if successful, false + an stderr message if something went wrong.
    bool Learn(const std::vector<Scalar>& values, std::vector<size_t> last_layer_experts);

private:
    size_t FindSplit(const std::vector<Scalar>& xs, const std::vector<size_t>& ys);
    void ComputeErrors(const std::vector<Scalar>& xs, const std::vector<size_t>& ys);
    size_t ComputeMaxError(const std::vector<Scalar>& xs, const std::vector<size_t>& ys);
    // Break down a list of sorted values into unique values and counts for each. Similar to the
    // np.uniques function.
    static std::pair<std::vector<Scalar>, std::vector<size_t>> Uniques(const std::vector<Scalar>& originals);
    static Segment FitMonotonic(std::array<double, 2> xs, std::array<double, 2> ys);
    static Segment FitLeastSq(const std::vector<Scalar>& xs, const std::vector<size_t>& ys);

    std::array<std::vector<Segment>, depth> parameters_;
    // separators_ is only stored if the RMI is read from a file. These are not produced when the RMI is
    // generated from the data.
    std::vector<int64_t> separators_;
    
    size_t nlayers_;
    Scalar min_val_;
    Scalar max_val_;
    size_t data_size_;
    // The error for the RMI on its training data.
    double avg_error_;
    double max_error_;
    // We need to make sure that there is no padding because we are
    // copying directly the binary from the file to the memory
    static_assert(sizeof(Segment) == 2 * sizeof(SegmentParam), "Padding in RMI Segment");

};

#include "../src/monotonic_RMI.hpp"

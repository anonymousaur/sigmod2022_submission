#include <algorithm>
#include "data_loading.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <limits>

template <size_t depth>
MonotonicRMI<depth>::MonotonicRMI()
    : parameters_(), avg_error_(0), max_error_(0) {
    }


/**
 * Construct a Locator from the weights file
 *
 * The weights file is expected to be a binary sequence of (scale, bias) tuples
 * stored in breath first search order
 *
 * @param filename the file containg the weigts
 */
template <size_t depth>
MonotonicRMI<depth>::MonotonicRMI(const std::string filename) {
    std::ifstream file;
    file.open(filename, std::ios::in | std::ios::binary);
    assert(file.good());

    std::string line;
    getline(file, line);
    std::istringstream iss(line);
    size_t nlayers;
    iss >> nlayers;
    // Makes sure that the RMI is what we expect.
    assert (nlayers == depth);

    int sizes[depth];   
    file.read((char*) sizes, depth * sizeof(int32_t));
    // The file is in BFS order and also stored in BFS order in memory
    // so we just have to copy it
    for (size_t i = 0; i < depth; ++i) {
        parameters_[i].resize(sizes[i]);
        file.read((char*) parameters_[i].data(), sizes[i] * sizeof(Segment));
    }
    // Read the separators.
    separators_ = std::vector<int64_t>(sizes[depth-1]+1);
    file.read((char *) separators_.data(), (sizes[depth-1] + 1) * sizeof(int64_t));
    data_size_ = 0;
}

template <size_t depth>
MonotonicRMI<depth>::MonotonicRMI(std::array<std::vector<Segment>, depth> parameters) {
    for (size_t i = 0; i < depth; i++) {
        parameters_[i] = parameters[i];
    }
}

template <size_t depth>
MonotonicRMI<depth>::MonotonicRMI(const std::vector<Scalar>& values, std::vector<size_t> layer_experts)
    : parameters_(), avg_error_(0), max_error_(0) {
    assert (std::is_sorted(values.begin(), values.end()));
    assert (Learn(values, layer_experts));
}

template <size_t depth>
void MonotonicRMI<depth>::SetMinMaxVals(Scalar min_val, Scalar max_val) {
    min_val_ = min_val;
    max_val_ = max_val;
}

template <size_t depth>
void MonotonicRMI<depth>::SetDataSize(size_t data_size) {
    data_size_ = data_size;
}

template <size_t depth>
double MonotonicRMI<depth>::At(Scalar val) const {
    double mapped = 0;
    Scalar value = std::min(max_val_, std::max(min_val_, (Scalar)val));
#ifdef RMI_EVAL_DEBUG
    std::cout << "Original Value: " << val << ", bounded value = " << value << std::endl;
#endif 
    for (size_t i = 0; i < depth; ++i) {
        size_t s = parameters_[i].size();
        const Segment &expert = parameters_[i][std::min<int>(s-1, std::max<int>(0, (int)mapped))];
#ifdef RMI_EVAL_DEBUG
        std::cout << "Expert index: " << mapped << " -> " << (int)mapped
            << ". segment weight: " << std::setprecision(15) << expert.weight << ", intercept: " << expert.intercept
            << ", next mapped = " << value * expert.weight + expert.intercept << std::endl;
#endif
        mapped = value * expert.weight + expert.intercept;
    }
#ifdef RMI_EVAL_DEBUG
    std::cout << "CDF result: " << mapped << std::endl;
#endif
    return mapped;
}

template <size_t depth>
double MonotonicRMI<depth>::Inverse(double yval) const {
    yval = std::max(0.0, std::min(std::nexttoward(1.0, 0.0), yval));
    const Segment& expert = parameters_[depth-1][yval * parameters_[depth-1].size()];
    return (yval - expert.intercept) / expert.weight;
}

template <size_t depth>
size_t MonotonicRMI<depth>::SizeInBytes() const {
    size_t size = 0;
    for (size_t i = 0; i < depth; i++) {
        size += parameters_[i].size() * sizeof(Segment);
    }
    size += 2 * sizeof(double);  // min_val_ and max_val_
    return size;
}

template <size_t depth>
Segment MonotonicRMI<depth>::GetLastLayerSegment(size_t segment_ix) {
    return parameters_[depth-1][segment_ix];
}

template <size_t depth>
void MonotonicRMI<depth>::SetLastLayerSegment(size_t segment_ix, const Segment& seg) {
    parameters_[depth-1][segment_ix] = seg;
}

template <size_t depth>
std::vector<int64_t> MonotonicRMI<depth>::GetSeparators() const {
    return separators_;
}

template <size_t depth>
size_t MonotonicRMI<depth>::NumLastLayerSegments() const {
    return parameters_[depth-1].size();
}

template <size_t depth>
std::vector<Scalar> MonotonicRMI<depth>::ComputeSeparators(const std::vector<Scalar>& values) const {
    std::vector<Scalar> seps(parameters_[depth-1].size()+1, std::numeric_limits<Scalar>::max());
    for (Scalar v : values) {
        double mapped = 0;
        Scalar value = std::min(max_val_+1, std::max(min_val_, v));
        for (size_t i = 0; i < depth-1; ++i) {
            const Segment &expert = parameters_[i][std::min((int)parameters_[i].size() - 1, std::max(0, (int)mapped))];
            mapped = value * expert.weight + expert.intercept;
        }
        size_t seps_ix = std::min((int)parameters_[depth-1].size()-1, std::max(0, (int)mapped));
        seps[seps_ix] = std::min(seps[seps_ix], value);
    }
    seps[parameters_[depth-1].size()] = max_val_ + 1;
    return seps;
}

template <size_t depth>
bool MonotonicRMI<depth>::Learn(const std::vector<Scalar>& values, std::vector<size_t> experts_per_layer) { 
    if (values.size() == 0) {
        for (size_t i = 0; i < depth; i++) {
            parameters_[i].push_back({0, 0});
        }
        min_val_ = 0;
        max_val_ = 0;
        avg_error_ = 0;
        max_error_ = 0;
        std::cout << "Warning: no data passed into RMI" << std::endl;
        return true;
    }
    std::pair<std::vector<Scalar>, std::vector<size_t>> uniques = Uniques(values);
    std::vector<Scalar> xs = uniques.first;
#ifdef RMI_DEBUG
    std::cout << "Fitting data with " << xs.size() << " unique values, " << values.size() << " total" << std::endl;
    std::cout << "Values: ";
    for (size_t i = 0; i < std::min<size_t>(xs.size(), 150); i++) {
        std::cout << xs[i] << " ";
        if (i % 10 == 0) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
#endif 
    min_val_ = xs[0];
    max_val_ = xs.back();
    // ys is the index in the array for the corresponding unique value x.
    // If there multiple of the same x-value, then ys reports the middle index.
    std::vector<size_t> ys(xs.size());
    size_t counts = 0;
    for (size_t i = 0; i < xs.size(); i++) {
        ys[i] = counts + uniques.second[i] / 2;
        counts += uniques.second[i];
    }
    // Cap the number of experts in the last layer if there aren't enough unique values.
    if (experts_per_layer.size() == 0) {
        std::cerr << "Building RMI without specifying number of experts per layer" << std::endl;
        return false;
    }
    if (experts_per_layer[0] != 1) {
        std::cerr << "The first RMI layer must have exactly one expert (root). Learn(..) called with " << experts_per_layer[0] << std::endl;
        return false;
    }
    size_t last_layer_size = std::max<size_t>(1, std::min<size_t>((1 + xs.size()) / 4, experts_per_layer.back()));    
    std::array<size_t, depth> sizes;
    sizes[0] = 1;
    sizes[depth-1] = last_layer_size;
    if (experts_per_layer.size() == 1) {
        // Autofill experts based on the last layer size.
        for (size_t i = 1; i < depth-1; i++) {
            sizes[i] = last_layer_size;
        }
    } else {
        // The number of layers should match the depth.
        if (experts_per_layer.size() != depth) {
            std::cerr << "Only " << experts_per_layer.size() << " expert layers specified, expected " << depth << std::endl;
            return false;
        }
        for (size_t i = 1; i < depth; i++) {
            sizes[i] = experts_per_layer[i];
        }
    }
    std::vector<double> splits {xs[0], xs.back() + 1e-3};
    for (size_t d = 0; d < depth-1; d++) {
#ifdef RMI_DEBUG
        std::cout << "Fitting layer " << d << " with " << sizes[d] << " experts " << std::endl;
#endif
        std::vector<size_t> alloc(splits.size()-1, 0);
        // The allocation for the next segment is how many points its reponsible for.
        size_t split_ix = 1;
        for (Scalar val : xs) {
            while (val >= splits[split_ix]) {
                split_ix++;
            }
            alloc[split_ix-1]++;
        }
        size_t target_range = sizes[d+1];
        double cumul_range = 0;
        size_t next_split_ix = 1;
        size_t split_x_ix = 0;
        parameters_[d].resize(sizes[d]);
        std::vector<double> next_splits(sizes[d+1]+1, xs[0]);
        for (size_t s = 0; s < alloc.size(); s++) {
            double next_range = cumul_range + target_range * ((double)alloc[s]) / xs.size();
            // Currently we only use monotonic segments in the middle of the tree (not the leaves).
            Segment seg = FitMonotonic({splits[s], splits[s+1]}, {cumul_range, next_range});
#ifdef RMI_DEBUG
            std::cout << "Fitting segment " << s << ": xs = " << splits[s] << " - " << splits[s+1]
                << ", ys = " << cumul_range << " - " << next_range << std::endl;
            std::cout << "Segment: " << std::setprecision(15) << seg.weight << " " << seg.intercept << std::endl;
#endif
            parameters_[d][s] = seg;
            cumul_range = next_range;

            // Find the inverses of all the integer values.
            while (next_split_ix <= cumul_range) {
                double xval = (next_split_ix - seg.intercept) / seg.weight;
                next_splits[next_split_ix] = xval; 
                next_split_ix++;
            }
        }
        // This will probably get set in the while loop above, but just be sure it's set correctly.
        next_splits[sizes[d+1]] = xs.back()+1e-3;
        splits = next_splits;
#ifdef RMI_DEBUG   
        std::cout << "Splits: ";
        for (size_t s : splits) {
            std::cout << s << " ";
        }
        std::cout << std::endl;
#endif
    }

    // Now form the last layer of the RMI
#ifdef RMI_DEBUG
    std::cout << "Fitting last layer with " << sizes[depth-1] << " experts" << std::endl;
#endif
    parameters_[depth-1].resize(sizes[depth-1]);
    parameters_[depth].resize(sizes[depth-1]);
    size_t last_ix = 0;
    for (size_t s = 0; s < splits.size()-1; s++) {
        size_t to_ix = last_ix;
        while (to_ix < xs.size() && xs[to_ix] < splits[s+1]) {
            to_ix++;
        }
        Scalar to_x = xs.back() + 1;
        if (to_ix <= xs.size()-1) {
            to_x = xs[to_ix];
        }
        size_t to_y_ix = std::min(to_ix, ys.size()-1);
        
        //Segment seg = FitLeastSq(std::vector<Scalar>(xs.begin() + last_ix, xs.begin() + to_ix),
        //                               std::vector<size_t>(ys.begin() + last_ix, ys.begin() + to_ix));
        Segment seg = FitMonotonic({splits[s], splits[s+1]}, {ys[last_ix], ys[std::min(to_ix, ys.size()-1)]});
        parameters_[depth-1][s] = seg;

        last_ix = to_ix;
    }
    ComputeErrors(xs, ys);
    return true;
}

template <size_t depth>
double MonotonicRMI<depth>::AvgError() const {
    return avg_error_;
}

template <size_t depth>
double MonotonicRMI<depth>::MaxError() const {
    return max_error_;
}

template <size_t depth>
void MonotonicRMI<depth>::ComputeErrors(const std::vector<Scalar>& xs, const std::vector<size_t>& ys) {
    double avg_error = 0;
    max_error_ = 0;
    for (size_t i = 0; i < xs.size(); i++) {
        double got = At(xs[i]);
        avg_error += fabs(got - ys[i]);
        max_error_ = std::max<double>(fabs(got - ys[i]), max_error_);
    }
    avg_error_ = avg_error /  xs.size();
}

template <size_t depth>
size_t MonotonicRMI<depth>::ComputeMaxError(const std::vector<Scalar>& xs, const std::vector<size_t>& ys) {
    size_t maxerr = 0;
    for (size_t i = 0; i < xs.size(); i++) {
        size_t got = At(xs[i]);
        size_t abs_err = got > ys[i] ? got - ys[i] : ys[i] - got;
        maxerr = std::max<size_t>(abs_err, maxerr);
    }
    return maxerr;
}

template <size_t depth>
std::pair<std::vector<Scalar>, std::vector<size_t>> MonotonicRMI<depth>::Uniques(const std::vector<Scalar>& originals) {
    std::vector<Scalar> uniques;
    std::vector<size_t> counts;
    Scalar last_val = -1;
    size_t cur_count = 0;
    for (Scalar val : originals) {
        if (val < last_val) {
            std::cout << "MonotonicRMI::Uniques: last val = " << last_val << ", val = " << val << std::endl;
        }
        assert (val >= last_val);
        if (val != last_val && last_val >= 0) {
            counts.push_back(cur_count);
            uniques.push_back(last_val);
            cur_count = 0;
        }
        last_val = val;
        cur_count++;
    }
    counts.push_back(cur_count);
    uniques.push_back(last_val);
    return std::make_pair(uniques, counts);
}

template <size_t depth>
Segment MonotonicRMI<depth>::FitMonotonic(std::array<double, 2> xs, std::array<double, 2> ys) {
    if (xs[0] == xs[1]) {
        return {0, (ys[0] + ys[1])/2.};
    }
    double slope = (ys[1] - ys[0])/(xs[1] - xs[0]);
    return { slope, ys[0] - slope * xs[0]};
}

template <size_t depth>
Segment MonotonicRMI<depth>::FitLeastSq(const std::vector<Scalar>& xs, const std::vector<size_t>& ys) {
    ldouble x = 0, x2 = 0, y = 0, y2 = 0, xy = 0;
    assert (xs.size() == ys.size());
    if (xs.size() == 0) {
        return {0, 0};
    } else if (xs.size() == 1) {
        return {0, ys[0]};
    }
    for (size_t i = 0; i < xs.size(); i++) {
        x += xs[i];
        x2 += xs[i] * xs[i];
        y += ys[i];
        y2 += ys[i] * ys[i];
        xy += xs[i] * ys[i];
    }
    ldouble size = xs.size();
    ldouble dx = x / size;
    ldouble dy = y / size;
    ldouble sxy = xy - dx*dy*size;
    ldouble sxx = x2 - dx*dx*size;
    ldouble syy = y2 - dy*dy*size;
    if (sxx <= 0) {
        std::cout << "sxx = " << (double)sxx << ", xs = " << xs[0] << " " << xs[1] << ", x2 = " << (double)x2 << ", dx = " << (double)dx << std::endl;
    }
    assert (sxx > 0);
    double slope = sxy / sxx;
    return {slope, dy - dx*slope};

}


#pragma once

#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

#include "types.h"
#include "utils.h"

class LinearModel {
  private:
    LinearModel() {}

  public:
    template <class ForwardIterator>
    static std::pair<double, double> Fit(ForwardIterator xbegin, ForwardIterator xend, ForwardIterator ybegin) {
        size_t n = std::distance(xbegin, xend);
        auto yend = ybegin + n;
        double c_x = std::accumulate(xbegin, xend, 0.0);
        double c_y = std::accumulate(ybegin, yend, 0.0);
        double c_xx = std::inner_product(xbegin, xend, xbegin, 0.0);
        double c_xy = std::inner_product(xbegin, xend, ybegin, 0.0);
        double c_yy = std::inner_product(ybegin, yend, ybegin, 0.0);

        double ssy = c_yy - c_y * c_y / n;
        double ssx = c_xx - c_x * c_x / n;
        double sxy = c_xy - c_x * c_y / n;
        double slope = sxy / ssx;
        double intercept = (c_y - slope * c_x) / n;
        return std::make_pair(slope, intercept);
    }
};



#include "just_sort_index.h"

#include <algorithm>
#include <parallel/algorithm>
#include <vector>
#include <cassert>
#include <string>
#include <iostream>

#include "types.h"
#include "utils.h"

template <size_t D>
void JustSortIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    data_size_ = std::distance(start, end);
    std::vector<std::pair<Scalar, size_t>> indices(data_size_);
    size_t i = 0;
    for (auto it = start; it != end; it++, i++) {
        Scalar ix_val = (*it)[column_];
        indices[i] = std::make_pair(ix_val, i);
    }

    // Sort by this array instead.
    std::stable_sort(indices.begin(), indices.end(),
        [](const std::pair<Scalar, size_t>& a, const std::pair<Scalar, size_t>& b) -> bool {
            return a.first < b.first;
        });

    std::vector<Point<D>> data_cpy(data_size_);
    const auto start_tmp = start;
    std::transform(indices.begin(), indices.end(), data_cpy.begin(),
            [start](const auto& ix_pair) -> Point<D> {
                return *(start + ix_pair.second);
                });

    std::copy(data_cpy.cbegin(), data_cpy.cend(), start);
}





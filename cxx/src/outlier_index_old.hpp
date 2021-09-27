#include "outlier_index.h"

#include <algorithm>
#include <parallel/algorithm>
#include <cassert>
#include <unordered_map>
#include <string>
#include <iostream>

#include "types.h"

template <size_t D>
SingleDimensionalOutlierIndex<D>::SingleDimensionalOutlierIndex(size_t dim, size_t ps, const std::vector<size_t>& outlier_list)
    : column_(dim), page_size_(ps), buckets_(), outlier_list_(outlier_list) {}

template <size_t D>
PhysicalIndexRange SingleDimensionalOutlierIndex<D>::PageRangeFor(Scalar start, Scalar end) {
    PhysicalIndexRange r;
    auto startit = pages_.upper_bound(start);
    if (startit != pages_.begin()) {
        // We need to find the previous page if it exists.
        startit--;
    }
    const Page p = startit->second;
    if (p.value_range.second < start) {
        // Special case optimization if there is no data in this range.
        r.start = p.index_range.second;
    } else {
        r.start = p.index_range.first;
    }
    r.end = pages_.upper_bound(end)->second.index_range.first; 
    return r;
}

template <size_t D>
std::vector<PhysicalIndexRange> SingleDimensionalOutlierIndex<D>::Ranges(const Query<D>& q) const {
    auto accessed = q.filters[column_];
    std::vector<PhysicalIndexRange> ranges;
    if (accessed.is_range) {
        ranges.reserve(2);
        ranges.push_back(PageRangeFor(accessed.range.first, accessed.range.second));
    } else {
        ranges.reserve(accessed.size()+1);
        for (Scalar val : accessed) {
            ranges.push_back(PageRangeFor(val, val));
        }
    }
    ranges.push_back(outlier_range_);
    return ranges;
}

template <size_t D>
void SingleDimensionalOutlierIndex<D>::ExtendOrTruncPage(Page* p, Scalar val, size_t minix, size_t maxix) {
    if (val == outlier_bucket_) {
        outlier_range_ = PhysicalIndexRange(minix, maxix, false);
    } else {
        cur_nvals = maxix - minix;
        cur_page_size = p.index_range.second - p.index_range.first;
        if (cur_page_size + cur_nvals > page_size_) {
            if (cur_page_size == 0) {
                p->index_range = {minix, maxix};
                p->value_range = {val, val};
                pages_.emplace(val, *p);
                *p = Page();
            } else {
                pages_.emplace(p->value_range.first, *p);
                *p = Page();
                p->index_range = {minix, maxix};
                p->value_range = {val, val};
            }
        } else {
            p->index_range.second = maxix;
            p->value_range.second = val;
        }
    }
}


template <size_t D>
void SingleDimensionalOutlierIndex<D>::Init(std::vector<Point<D>>* data) {
    std::vector<std::pair<Scalar, size_t>> indices(data->size());
    size_t outlier_ix = 0;
    for (size_t i = 0; i < data->size(); i++) {
        Scalar ix_val = 0;
        if (outlier_ix >= outlier_list_.size() || i < outlier_list_[outlier_ix]) {
            ix_val = data->at(i)[column_];
        } else {
            ix_val = outlier_bucket_;
            outlier_ix++;
        }
        indices[i] = std::make_pair(ix_val, i);
    }

    // Sort by this array instead.
    std::sort(indices.begin(), indices.end(),
        [](const std::pair<Scalar, size_t>& a, const std::pair<Scalar, size_t>& b) -> bool {
            return a.first < b.first;
        });
    std::vector<Point<D>> data_cpy(data->size());
    std::transform(indices.begin(), indices.end(), data_cpy.begin(),
            [data](const auto& ix_pair) -> Point<D> {
                return data->at(ix_pair.second);
                });

    // Run through sorted data to build the index
    Scalar cur_ix_val = SCALAR_MIN;
    size_t cur_min_ix = 0;
    size_t cur_max_ix = 0;
    Page page;
    for (const auto item : indices) {
        if (cur_ix_val < item.first) {
            ExtendOrTruncPage(&page, cur_ix_val, cur_min_ix, cur_max_ix);
            cur_min_ix = cur_max_ix;
            cur_ix_val = item.first;
        }
        cur_max_ix++;
    }
    // Get the last one too
    ExtendOrTruncPage(&page, cur_ix_val, cur_min_ix, cur_max_ix);
    // If the page isn't empty, add it to the index.
    if (page.index_range.second > page.index_range.first) {
        pages_.emplace(page.value_range.first, page);
    }

    std::cout << "Index has " << buckets_.size() << " buckets + outliers" << std::endl;
    std::cout << "Outlier range: " << outlier_range_.start << " - " << outlier_range_.end << std::endl;
    outlier_list_.clear();    
    data->assign(data_cpy.begin(), data_cpy.end());
    ready_ = true;
}


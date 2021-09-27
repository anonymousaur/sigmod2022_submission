#include "primary_btree_index.h"

#include <algorithm>
#include <parallel/algorithm>
#include <cassert>
#include <map>
#include <string>
#include <iostream>

#include "types.h"
#include "utils.h"

template <size_t D>
PrimaryBTreeIndex<D>::PrimaryBTreeIndex(size_t dim, size_t ps)
    : PrimaryIndexer<D>(dim), column_(dim), page_size_(ps), pages_() { assert(ps > 0); }

template <size_t D>
Range<PhysicalIndex> PrimaryBTreeIndex<D>::PageRangeFor(Scalar start, Scalar end) const {
    Range<PhysicalIndex> r;
    auto startit = pages_.upper_bound(start);
    if (startit != pages_.begin()) {
       startit--;
    }
    // Now, startit points to the first page with first element <= start. 
    const Page p = startit->second;
    if (p.value_range.second < start) {
        // If this page actually doesn't have any matching elements, skip it.
        r.start = p.index_range.second;
    } else {
        // Otherwise, include the whole page.
        r.start = p.index_range.first;
    }
    r.end = pages_.upper_bound(end)->second.index_range.first; 
    return r;
}

template <size_t D>
Set<PhysicalIndex> PrimaryBTreeIndex<D>::IndexRanges(Query<D>& q) {
    auto accessed = q.filters[column_];
    if (!accessed.present || pages_.empty()) {
        // No actionable filter on the data, so scan everything.
        return Set<PhysicalIndex>({{0, data_size_}}, List<PhysicalIndex>());
    }
    Ranges<PhysicalIndex> ranges;
    if (accessed.is_range) {
        ranges.reserve(accessed.ranges.size());
        size_t added = 0;
        for (ScalarRange r : accessed.ranges) {
            const Range<PhysicalIndex> pr = PageRangeFor(r.first, r.second);
            if (added > 0 && pr.start == ranges[added-1].end) {
                ranges[added-1].end = pr.end;
            } else if (pr.end > pr.start) {
                ranges.push_back(pr);
                added++;
            }
        }
    } else {
        ranges.reserve(accessed.values.size());
        size_t added = 0;
        for (Scalar val : accessed.values) {
            const Range<PhysicalIndex> pr = PageRangeFor(val, val);
            if (added > 0 && pr.start == ranges[added-1].end) {
                ranges[added-1].end = pr.end;
            } else if (pr.end > pr.start) {
                ranges.push_back(pr);
                added++;
            }
        }
    }
    return Set<PhysicalIndex>(ranges, List<PhysicalIndex>());
}

template <size_t D>
void PrimaryBTreeIndex<D>::ExtendOrTruncPage(Page* p, Scalar val, size_t minix, size_t maxix) {
    size_t cur_nvals = maxix - minix;
    size_t cur_page_size = p->index_range.second - p->index_range.first;
    if (cur_page_size + cur_nvals > page_size_) {
        if (cur_page_size > 0) {
            pages_.emplace(p->value_range.first, *p);
            *p = Page();
        }
        p->index_range = {minix, maxix};
        p->value_range = {val, val};
    } else {
        p->index_range.second = maxix;
        p->value_range.second = val;
    }
}


template <size_t D>
bool PrimaryBTreeIndex<D>::Init(PointIterator<D> start, PointIterator<D> end) {
    size_t s = std::distance(start, end);
    data_size_ = s;
    std::vector<std::pair<Scalar, size_t>> indices(s);
    size_t i = 0;
    bool modified = false;
    Scalar prev = 0;
    for (auto it = start; it != end; it++, i++) {
        indices[i] = std::make_pair((*it)[column_], i);
        modified |= it != start && prev > indices[i].first;
        prev = indices[i].first;
    }


    // Sort by this array instead.
    std::stable_sort(indices.begin(), indices.end(),
        [](const std::pair<Scalar, size_t>& a, const std::pair<Scalar, size_t>& b) -> bool {
            return a.first < b.first;
        });
    std::vector<Point<D>> data_cpy(s);
    std::transform(indices.begin(), indices.end(), data_cpy.begin(),
            [start](const auto& ix_pair) -> Point<D> {
                return *(start + ix_pair.second);
                });

    // Run through sorted data to build the index
    bool first = true;
    Scalar cur_ix_val = SCALAR_MIN;
    size_t cur_min_ix = 0;
    size_t cur_max_ix = 0;
    Page page;
    for (const auto item : indices) {
        if (first) {
            page.value_range.first = item.first;
            cur_ix_val = item.first;
            first = false;
        }
        if (cur_ix_val < item.first) {
            ExtendOrTruncPage(&page, cur_ix_val, cur_min_ix, cur_max_ix);
            cur_min_ix = cur_max_ix;
            cur_ix_val = item.first;
        }
        cur_max_ix++;
    }
    // Get the last value too.
    ExtendOrTruncPage(&page, cur_ix_val, cur_min_ix, cur_max_ix);
    // If the page isn't empty, add it to the index.
    if (page.index_range.second > page.index_range.first) {
        pages_.emplace(page.value_range.first, page);
    }

    std::cout << "Index has " << pages_.size() << " buckets" << std::endl;
    std::copy(data_cpy.begin(), data_cpy.end(), start);
    ready_ = true;
    return modified;
}


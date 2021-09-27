#include "merge_utils.h"
#include "utils.h"

#include <algorithm>


template <typename T>
Ranges<T> MergeUtils::Intersect(const Ranges<T>& first, const Ranges<T>& second) {
    // Assumes both ranges are already sorted.
    if (first.empty() || second.empty()) {
        return {};
    }
    Ranges<T> final_ranges;
    size_t i = 0, j = 0;
    size_t count = 0;
    T cur_first = first[0].start, cur_second = second[0].start;
    bool i_start = true, j_start = true;
    bool in_range = false;
    Range<T> cur_range;
    while (i < first.size() && j < second.size()) {
        T cur_first, cur_second, popped;
        if (i_start) { cur_first = first[i].start; }
        else { cur_first = first[i].end; }
        
        if (j_start) { cur_second = second[j].start; }
        else { cur_second = second[j].end; }
        
        // Here, we don't care about the case when they're equal.
        // Since ranges in each input vector are not contiguous, if there's an equality here, it
        // won't change the final answer.
        if (cur_first < cur_second) {
            popped = cur_first;
            if (i_start) {
                count++;
                i_start = false;
            }
            else {
                count--;
                i++;
                i_start = true;
            }
        } else {
            popped = cur_second;
            if (j_start) {
                count++;
                j_start = false;
            }
            else {
                count--;
                j_start = true;
                j++;
            }
        }
        if (count == 2) {
            assert (!in_range);
            in_range = true;
            cur_range.start = popped;
        } else if (in_range) {
            assert (count == 1);
            cur_range.end = popped;
            final_ranges.push_back(cur_range);
            in_range = false;
        }
    }
    assert (count < 2);
    return final_ranges;
}

std::vector<ScalarRange> MergeUtils::Intersect(const std::vector<ScalarRange>& first, const std::vector<ScalarRange>& second) {
    // Assumes both ranges are already sorted.
    std::vector<ScalarRange> final_ranges;
    size_t i = 0, j = 0;
    size_t count = 0;
    Scalar cur_first = first[0].first, cur_second = second[0].first;
    bool i_start = true, j_start = true;
    bool in_range = false;
    ScalarRange cur_range;
    while (i < first.size() && j < second.size()) {
        Scalar cur_first, cur_second, popped;
        if (i_start) { cur_first = first[i].first; }
        else { cur_first = first[i].second; }
        
        if (j_start) { cur_second = second[j].first; }
        else { cur_second = second[j].second; }
        
        // Here, we don't care about the case when they're equal.
        // Since ranges in each input vector are not contiguous, if there's an equality here, it
        // won't change the final answer.
        if (cur_first < cur_second) {
            popped = cur_first;
            if (i_start) {
                count++;
                i_start = false;
            }
            else {
                count--;
                i++;
                i_start = true;
            }
        } else {
            popped = cur_second;
            if (j_start) {
                count++;
                j_start = false;
            }
            else {
                count--;
                j_start = true;
                j++;
            }
        }
        if (count == 2) {
            assert (!in_range);
            in_range = true;
            cur_range.first = popped;
        } else if (in_range) {
            assert (count == 1);
            cur_range.second = popped;
            final_ranges.push_back(cur_range);
            in_range = false;
        }
    }
    assert (count < 2);
    return final_ranges;
}

template <typename T>
List<T> MergeUtils::Intersect(const List<T>& list1, const List<T>& list2) {
    std::cout << "Intersecting two KeyLists with sizes " << list1.size() << ", " << list2.size() << std::endl;
    List<T> output;
    output.reserve(std::min(list1.size(), list2.size()));
    std::set_intersection(list1.begin(), list1.end(), list2.begin(), list2.end(),
            std::back_inserter(output));
    std::cout << "Intersect output = " << output.size() << std::endl;
    return output;
}

//Keep - change to keyset
template <typename T>
Set<T> MergeUtils::Intersect(const Set<T>& set1, const Set<T>& set2) {
    // Assumes both ranges are already sorted.
    Ranges<T> final_ranges;
    List<T> final_list;
    final_list.reserve(set1.list.size() + set2.list.size());
    auto first_list_it = set1.list.cbegin();
    auto first_range_it = set1.ranges.cbegin();
    auto second_list_it = set2.list.cbegin();
    auto second_range_it = set2.ranges.cbegin();
    // We're just intersecting the lists here.    
    while (true) {
        bool first_list_done = first_list_it == set1.list.cend();
        bool second_list_done = second_list_it == set2.list.cend();
        if (first_list_done && second_list_done) {
            break;
        }
        if (!first_list_done && (second_list_done || *first_list_it < *second_list_it)) {
            bool second_range_ready = second_range_it != set2.ranges.cend();
            if (second_range_ready && *first_list_it >= second_range_it->end) {
                second_range_it++;
            } else if (second_range_ready && *first_list_it >= second_range_it->start) {
                final_list.push_back(*first_list_it);
                first_list_it++;
            } else {
                first_list_it++;
            }
        } else if (!second_list_done && (first_list_done || *second_list_it < *first_list_it)) {
            bool first_range_ready = first_range_it != set1.ranges.cend();
            if (first_range_ready && *second_list_it >= first_range_it->end) {
                first_range_it++;
            } else if (first_range_ready && *second_list_it >= first_range_it->start) {
                final_list.push_back(*second_list_it);
                second_list_it++;
            } else {
                second_list_it++;
            }
        } else {
            // list indexes are equal to each other.
            final_list.push_back(*first_list_it);
            first_list_it++;
            second_list_it++;
        }
    }
    final_list.shrink_to_fit();
    return { MergeUtils::Intersect(set1.ranges, set2.ranges), final_list };
}

template <typename T>
Set<T> MergeUtils::Union(const Ranges<T>& ranges, const List<T>& list) {
    // Just eliminate the indexes that fall inside one of the ranges. Assumes both are sorted.
    if (ranges.size() == 0) {
        return {{}, list};
    }
    size_t cur_range_ix = 0;
    size_t list_ix = 0;
    List<T> pruned;
    pruned.reserve(list.size());
    while (list_ix < list.size() && cur_range_ix < ranges.size()) {
        size_t index = list[list_ix];
        if (index < ranges[cur_range_ix].start) {
            pruned.push_back(index);
            list_ix++;
        } else if (index >= ranges[cur_range_ix].end) {
            cur_range_ix++;
        } else {
            // The index is in a range.
            list_ix++;
        }
    }
    if (list_ix < list.size()) {
        pruned.insert(pruned.end(), list.begin() + list_ix, list.end());
    }
    pruned.shrink_to_fit();
    return {ranges, pruned};
}

template <typename T>
// Takes a priority queue of pointers to IndexLists sorted by their sizes.
List<T> MergeUtils::Union(const std::vector<const List<T>*> ix_lists) {
    size_t max_size = 0;
    std::vector<size_t> breaks;
    size_t num_lists = ix_lists.size();
    breaks.reserve(num_lists+1);
    breaks.push_back(0);
    for (auto ixl : ix_lists) {
        max_size += ixl->size();
        breaks.push_back(max_size);
    }
    List<T> all_ixs;
    all_ixs.reserve(max_size);
    for (auto ixl : ix_lists) {
        all_ixs.insert(all_ixs.end(), ixl->begin(), ixl->end());
    }
    std::vector<size_t> next_breaks;
    next_breaks.reserve(num_lists);
    // Perform a series of in-place merges until the entire list is sorted.
    while (breaks.size() > 2) {
        next_breaks.clear();
        next_breaks.push_back(0);
        for (size_t i = 2; i < breaks.size(); i += 2) {
            std::inplace_merge(all_ixs.begin()+breaks[i-2],
                               all_ixs.begin()+breaks[i-1],
                               all_ixs.begin()+breaks[i]);
            next_breaks.push_back(breaks[i]);
        }
        // Even sized breaks means we have an odd number of ranges.
        if ((breaks.size() & 1) == 0) {
           next_breaks.push_back(breaks.back()); 
        }
        std::swap(breaks, next_breaks);
    }
    return all_ixs;
}

template <class ForwardIterator>
std::vector<ScalarRange> MergeUtils::Coalesce(ForwardIterator begin, ForwardIterator end) {
    std::vector<ScalarRange> ranges;
    ScalarRange cur_range = {std::numeric_limits<Scalar>::lowest(), std::numeric_limits<Scalar>::lowest()};
    for (auto it = begin; it != end; it++) {
        if (it->first <= cur_range.second) {
            cur_range.second = std::max(cur_range.second, it->second);
        } else {
            if (cur_range.second > cur_range.first) {
                ranges.push_back(cur_range);
            }
            cur_range = {it->first, it->second};
        }
    }
    ranges.push_back(cur_range);
    return ranges;
}


#include "single_column_rewriter.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>

#include "utils.h"

template <size_t D>
SingleColumnRewriter<D>::SingleColumnRewriter(const std::string& filename) 
    : cmap_continuous_(), cmap_categorical_(), mapped_dim_(), target_dim_() {
    Load(filename);
}

template <size_t D>
void SingleColumnRewriter<D>::Load(const std::string& filename) {
    std::ifstream file(filename);
    assert (file.is_open());
    std::string line;
    std::getline(file, line);
    if (line == "categorical") {
        LoadCategorical(file);
    } else if (line == "continuous") {
        LoadContinuous(file);
    } else if (line == "testing") {
        // Do nothing.    
    } else {
        std::cerr << "ERROR: Unrecognized header in rewriter file: " << line << std::endl;
        abort();
    }
    file.close();
}

/*
 * The format of the file is:
 *  | continuous
 *  | mapped_dim target_dim
 *  | mapped_bucket1.start mapped_bucket1.end num_target_ranges
 *  | target_range.start target_range.end
 *  | target_range.start target_range.end
 *  | ...
 *  | mapped_bucket2.start mapped_bucket2.end num_target_ranges2
 *  | target_range.start target_range.end
 *  ...
 *  Note: mapped_bucket{i}.start = mapped_bucket_{i-1}.end
 */
template <size_t D>
void SingleColumnRewriter<D>::LoadContinuous(std::istream& file) {
    std::string line;
    std::getline(file, line);
    std::istringstream head(line);
    
    head >> mapped_dim_ >> target_dim_;
    assert (mapped_dim_ != target_dim_
           && mapped_dim_ < D && target_dim_ < D);

    Scalar prev_bucket_end = std::numeric_limits<Scalar>::lowest();
    while (!file.eof()) {
        Scalar bucket_start, bucket_end;
        int nranges;
        if (!(file >> bucket_start >> bucket_end >> nranges)) {
            break;
        }
        // Make sure buckets are in increasing order.
        if (bucket_end <= prev_bucket_end) {
            std::cout << "Mapped buckets are not specified in increasing order: "
                << "previous bucket ends at " << prev_bucket_end << " but is followed by bucket at "
                << bucket_end << std::endl;
            abort();
        }
        std::vector<ScalarRange> ranges(nranges);
        for (size_t i = 0; i < nranges; i++) {
            Scalar start_range, end_range;
            file >> start_range >> end_range;
            ranges[i] = {start_range, end_range};
            if (i > 0) {
                // Make sure the target ranges are given to us in increasing order.
                if (start_range <= ranges[i-1].second) {
                    std::cout << "Target ranges for bucket [" << bucket_start
                        << ", " << bucket_end
                        << "] are not in increasing order: got ... ["
                        << ranges[i-1].first << ", " << ranges[i-1].second << "] ["
                        << start_range << ", " << end_range << "]" << std::endl;
                    abort();
                }
            }
        }
        prev_bucket_end = bucket_end;
        assert(cmap_continuous_.upper_bound(bucket_end) == cmap_continuous_.end());
        cmap_continuous_[bucket_end] = ranges;
    }
    std::cout << "Rewriter has " << cmap_continuous_.size() << " mapping entries" << std::endl;
}

/**
 * The format of the file is:
 *  | categorical
 *  | mapped_dimension target_dimension
 *  | mapped_value1 num_targets
 *  | <list of target values>
 *  | ...
 */
template <size_t D>
void SingleColumnRewriter<D>::LoadCategorical(std::istream& file) {
    size_t num_entries = 0;
    // These getlines make sure we discard the newline after each line when parsing.
    std::string line;
    std::getline(file, line);
    std::istringstream head(line);

    head >> mapped_dim_ >> target_dim_;
    assert (mapped_dim_ != target_dim_
           && mapped_dim_ < D && target_dim_ < D);
    
    while (!file.eof()) {
        std::getline(file, line);
        if (line.empty()) {
            break;
        }    
        size_t num_targets;
        Scalar mapped_val;
        std::istringstream spec(line);
        spec >> mapped_val >> num_targets;

        std::getline(file, line);
        std::istringstream liness(line);
        Scalar target_val;
        std::vector<Scalar> targets;
        targets.reserve(num_targets);
        while (liness >> target_val) {
            targets.push_back(target_val);
        }
        assert (cmap_categorical_.find(mapped_val) == cmap_categorical_.end());
        assert (targets.size() == num_targets);
        cmap_categorical_[mapped_val] = targets;
    }
    std::cout << "Rewriter has " << cmap_categorical_.size() << " mapping entries" << std::endl;
}

template <size_t D>
std::vector<ScalarRange> SingleColumnRewriter<D>::UnionSortedRanges(const std::vector<ScalarRange>& first,
        const std::vector<ScalarRange>& second) const {
    // Assumes both ranges are already sorted.
    std::vector<ScalarRange> final_ranges;
    size_t i = 0, j = 0;
    size_t count = 0;
    Scalar cur_first = first[0].first, cur_second = second[0].first;
    bool i_start = true, j_start = true;
    bool in_range = false;
    ScalarRange cur_range;
    while (i < first.size() || j < second.size()) {
        Scalar cur_first = std::numeric_limits<Scalar>::max(),
               cur_second = std::numeric_limits<Scalar>::max(),
               popped = 0;
        if (i < first.size()) {
            if (i_start) { cur_first = first[i].first; }
            else { cur_first = first[i].second; }
        }
        if (j < second.size()) {
            if (j_start) { cur_second = second[j].first; }
            else { cur_second = second[j].second; }
        }
        // If cur_first == cur_second, we want to prefer the one that is a start
        // of a range.
        if (cur_first < cur_second || (cur_first == cur_second && i_start)) {
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
        if (count == 0) {
            // We should have had a range started at this point. 
            assert (in_range);
            cur_range.second = popped;
            final_ranges.push_back(cur_range);
            in_range = false;
        } else if (!in_range) {
            // We have just started a new range.
            assert (count == 1);
            cur_range.first = popped;
            in_range = true;
        }
    }
    assert (count == 0);
    return final_ranges;
}

template <size_t D>
std::vector<ScalarRange> SingleColumnRewriter<D>::IntersectSortedRanges(const std::vector<ScalarRange>& first,
        const std::vector<ScalarRange>& second) const {
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

template <size_t D>
QueryFilter SingleColumnRewriter<D>::RewriteRangeFilter(const QueryFilter& qf_mapped,
        const QueryFilter& qf_target) const {
    if (qf_mapped.ranges.size() > 1) {
        std::cerr << "Query cannot have more than one range in each dimension, found "
            << qf_mapped.ranges.size() << std::endl;
        abort();
    }
    ScalarRange qrange = qf_mapped.ranges[0];
    auto start = cmap_continuous_.lower_bound(qrange.first);
    auto end = cmap_continuous_.lower_bound(qrange.second);
    end++;
    // For each mapped column, we union the ranges, then intersect it with any existing range in the
    // target dimension's filter.
    std::vector<ScalarRange> ranges;
    for (auto it = start; it != end; it++) {
        ranges = UnionSortedRanges(ranges, it->second);     
    }
    if (qf_target.present) {
        // We don't currently handle mapping from continuous to categorical.
        assert (qf_target.is_range);
        ranges = IntersectSortedRanges(ranges, qf_target.ranges);
    }
    QueryFilter q = {.present = true, .is_range = true, .ranges = ranges};
    return q;
}

template <size_t D>
QueryFilter SingleColumnRewriter<D>::RewriteValueFilter(const QueryFilter& qf_mapped,
        const QueryFilter& qf_target) const {
    std::set<Scalar> targets;
    for (Scalar mval : qf_mapped.values) {
        auto loc = cmap_categorical_.find(mval);
        if (loc != cmap_categorical_.end()) {
            targets.insert(loc->second.begin(), loc->second.end());
        }
    }
    std::vector<Scalar> merged;
    if (!qf_target.present) {
        merged.insert(merged.end(), targets.begin(), targets.end());
    } else {
        // We don't currently handle maps between categorical and continuous dimensions.
        assert (!qf_target.is_range);
        // Intersect the two.
        auto targets_it = targets.begin();
        size_t q_ix = 0;
        auto q_targets = qf_target.values;
        while (targets_it != targets.end() && q_ix < q_targets.size()) {
            if (*targets_it < q_targets[q_ix]) {
                targets_it++;
            } else if (q_targets[q_ix] < *targets_it) {
                q_ix++;
            } else {
                merged.push_back(*targets_it);
                targets_it++;
                q_ix++;
            }
        }
    }
    QueryFilter qf = {.present = true, .is_range = false, .values = merged};
    return qf;
}

template <size_t D>
List<Key> SingleColumnRewriter<D>::Rewrite(Query<D>& q) {
    // Deep copy the query filters.
    /*for (int i = 0; i < D; i++) {
        rewritten.filters[i] = q.filters[i];
        if (rewritten.filters[i].present) {
            if (rewritten.filters[i].is_range) {
                rewritten.filters[i].ranges = q.filters[i].ranges;
            } else {
                rewritten.filters[i].values = q.filters[i].values;
            }
        }
    }*/
    if (q.filters[mapped_dim_].present) {
        if (q.filters[mapped_dim_].is_range) {
            q.filters[target_dim_] = RewriteRangeFilter(
                    q.filters[mapped_dim_], q.filters[target_dim_]);
        } else {
            q.filters[target_dim_] = RewriteValueFilter(
                    q.filters[mapped_dim_], q.filters[target_dim_]);
        }
    }
    AssertWithMessage(false, "SingleColumnRewriter does not support auxiliary indexes");
    return {};
}



/*
 * Maps a single column to another, A -> B. Any values of A that appear in the query are mapped to
 * their corresponding values of B, which is then intersected with the existing B values in the
 * query to form the full rewritten query.
 */

#pragma once

#include <string>

#include "types.h"
#include "rewriter.h"
#include <vector>

template <size_t D>
class SingleColumnRewriter : public Rewriter<D> {
  public:
    // Loads the one-to-one rewriter from a file.
    SingleColumnRewriter(const std::string& filename);

    size_t MappedDim() const {
        return mapped_dim_;
    }
    
    size_t TargetDim() const {
        return target_dim_;
    }

    size_t Size() const override {
        size_t s = 0;
        if (cmap_categorical_.size() > 0) {
            for (auto it = cmap_categorical_.cbegin(); it != cmap_categorical_.cend(); it++) {
                s += sizeof(Scalar) + it->second.size() * sizeof(Scalar);
            }
        }
        if (cmap_continuous_.size() > 0) {
            for (auto it = cmap_continuous_.cbegin(); it != cmap_continuous_.cend(); it++) {
                s += sizeof(Scalar) + it->second.size() * sizeof(ScalarRange);
            }
        }
        s += 2 * sizeof(size_t);
        return s;
    }

    List<Key> Rewrite(Query<D>& q) override;

    void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override {}
        
    void Load(const std::string& filename);

    std::vector<ScalarRange> UnionSortedRanges(
            const std::vector<ScalarRange>& first,
            const std::vector<ScalarRange>& second) const;

    std::vector<ScalarRange> IntersectSortedRanges(
            const std::vector<ScalarRange>& first,
            const std::vector<ScalarRange>& second) const;

  protected:
    void LoadContinuous(std::istream& file);
    void LoadCategorical(std::istream& file);

    QueryFilter RewriteRangeFilter(const QueryFilter& mapped, const QueryFilter& target) const;
    QueryFilter RewriteValueFilter(const QueryFilter& mapped, const QueryFilter& target) const;



  private:
    // For categorical variables, we map each value to a set of target values.
    // Unioning is simple to do with a set. 
    std::map<Scalar, std::vector<Scalar>> cmap_categorical_;
    // For continuous variables, each value (marking the *end* of a column) is mapped to a sequence
    // of target value ranges. Unioning is done by a linear merge algorithm.
    std::map<Scalar, std::vector<ScalarRange>> cmap_continuous_;
    // The rewriter maps values in mapped_dim_ to values in target_dim_ and adds those target_dim_
    // values to the query.
    size_t mapped_dim_;
    size_t target_dim_;
};

#include "../src/single_column_rewriter.hpp"

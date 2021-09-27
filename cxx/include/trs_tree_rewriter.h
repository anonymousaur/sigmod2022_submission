/*
 * Dispatches to multiple linear rewriters on different columns.
 */

#pragma once

#include <string>
#include <vector>

#include "types.h"
#include "rewriter.h"
#include "trs_node.h"
#include "secondary_indexer.h"

template <size_t D>
class TRSTreeRewriter : public Rewriter<D> {
  public:
    TRSTreeRewriter(size_t mapped, size_t target, float err_bd);

    void SetAuxiliaryIndex(std::unique_ptr<SecondaryBTreeIndex<D>> index) {
        outlier_index_ = std::move(index);
    }

    void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) override;

    List<Key> Rewrite(Query<D>& q) override;

    void WriteOutliers(const List<Key>& outliers);

    size_t Size() const override {
        size_t s = trs_root_->Size();
        if (outlier_index_ != nullptr) {
            s += outlier_index_->Size();
        }
        return s;
    }    

  private:
    std::unique_ptr<TRSNode> trs_root_;
    // The rewriter maps values in mapped_dim_ to values in target_dim_ and adds those target_dim_
    // values to the query.
    size_t mapped_dim_;
    size_t target_dim_;
    size_t data_size_;
    std::unique_ptr<SecondaryBTreeIndex<D>> outlier_index_;
};

#include "../src/trs_tree_rewriter.hpp"

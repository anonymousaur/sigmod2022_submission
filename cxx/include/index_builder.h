#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "indexer.h"
#include "dummy_index.h"
#include "just_sort_index.h"
#include "composite_index.h"
#include "correlation_indexer.h"
#include "secondary_indexer.h"
#include "primary_indexer.h"
#include "secondary_btree_index.h"
#include "binary_search_index.h"
#include "primary_btree_index.h"
#include "flood_index.h"
#include "combined_correlation_index.h"
#include "mapped_correlation_index.h"
#include "outlier_index.h"
#include "bucketed_secondary_index.h"
#include "octree_index.h"
#include "rewriter.h"
#include "linear_model_rewriter.h"
#include "trs_tree_rewriter.h"
#include "measure_beta_index.h"

/*
 * Given a file as an index specification, builds and returns the appropriate index.
 */
template <size_t D>
class IndexBuilder {
  public:
    IndexBuilder() {}

    std::shared_ptr<PrimaryIndexer<D>> Build(std::string index_spec);

  protected:
    // Responsible for calling the correct builder for the next index.
    std::unique_ptr<Indexer<D>> Dispatch(std::ifstream& spec, bool root = false);

    // Index specific builders.
    std::unique_ptr<JustSortIndex<D>> BuildJustSortIndex(std::ifstream& spec);
    std::unique_ptr<DummyIndex<D>> BuildDummyIndex(std::ifstream& spec);
    std::unique_ptr<CompositeIndex<D>> BuildCompositeIndex(std::ifstream& spec);
    std::unique_ptr<CombinedCorrelationIndex<D>> BuildCombinedCorrelationIndex(std::ifstream& spec);
    std::unique_ptr<MappedCorrelationIndex<D>> BuildMappedCorrelationIndex(std::ifstream& spec);
    std::unique_ptr<SecondaryBTreeIndex<D>> BuildSecondaryBTreeIndex(std::ifstream& spec);
    std::unique_ptr<BinarySearchIndex<D>> BuildBinarySearchIndex(std::ifstream& spec);
    std::unique_ptr<PrimaryBTreeIndex<D>> BuildPrimaryBTreeIndex(std::ifstream& spec);
    std::unique_ptr<OctreeIndex<D>> BuildOctreeIndex(std::ifstream& spec);
    std::unique_ptr<FloodIndex<D>> BuildFloodIndex(std::ifstream& spec);
    std::unique_ptr<BucketedSecondaryIndex<D>> BuildBucketedSecondaryIndex(std::ifstream& spec);
    std::unique_ptr<LinearModelRewriter<D>> BuildLinearModelRewriter(std::ifstream& spec);
    std::unique_ptr<TRSTreeRewriter<D>> BuildTRSTreeRewriter(std::ifstream& spec);
    std::unique_ptr<MeasureBetaIndex<D>> BuildMeasureBetaIndex(std::ifstream& spec);
};

#include "../src/index_builder.hpp"

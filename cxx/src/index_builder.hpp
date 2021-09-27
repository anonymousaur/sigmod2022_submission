#include "index_builder.h"

#include "utils.h"

template <size_t D>
std::shared_ptr<PrimaryIndexer<D>> IndexBuilder<D>::Build(std::string spec) {
    std::ifstream f(spec);
    assert (f.is_open());
    auto base_ptr = Dispatch(f, true);
    AssertWithMessage(base_ptr->Type() == IndexerType::Primary, "Root index must be a primary index");
    std::shared_ptr<PrimaryIndexer<D>> indexer(dynamic_cast<PrimaryIndexer<D>*>(base_ptr.release()));
    return indexer;
}

template <size_t D>
std::unique_ptr<Indexer<D>> IndexBuilder<D>::Dispatch(std::ifstream& spec, bool root) {
    std::string next_index;
    spec >> next_index;
    if (next_index == "DummyIndex") {
        return BuildDummyIndex(spec);
    } else if (next_index == "CompositeIndex") { 
        return BuildCompositeIndex(spec);
    } else if (next_index == "CombinedCorrelationIndex") {
        assert (!root);
        return BuildCombinedCorrelationIndex(spec);
    } else if (next_index == "MappedCorrelationIndex") {
        assert (!root);
        return BuildMappedCorrelationIndex(spec);
    } else if (next_index == "SecondaryBTreeIndex") {
        assert (!root);
        return BuildSecondaryBTreeIndex(spec);
    } else if (next_index == "BinarySearchIndex") {
        return BuildBinarySearchIndex(spec);
    } else if (next_index == "PrimaryBTreeIndex") {
        return BuildPrimaryBTreeIndex(spec);
    } else if (next_index == "OctreeIndex") {
        return BuildOctreeIndex(spec);
    } else if (next_index == "FloodIndex") {
        return BuildFloodIndex(spec);
    } else if (next_index == "BucketedSecondaryIndex") {
        return BuildBucketedSecondaryIndex(spec);
    } else if (next_index == "LinearModelRewriter") {
        return BuildLinearModelRewriter(spec);
    } else if (next_index == "TRSTreeRewriter") {
        return BuildTRSTreeRewriter(spec);
    } else if (next_index == "MeasureBetaIndex") {
        return BuildMeasureBetaIndex(spec);
    } else if (next_index == "}") {
        // Case to deal with variable length indexes
        return nullptr;
    }
    AssertWithMessage(false, "Index type " + next_index + " not recognized");
    return nullptr;    
}

template <size_t D>
std::unique_ptr<DummyIndex<D>> IndexBuilder<D>::BuildDummyIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    spec >> parens.first  >> parens.second;
    AssertWithMessage(parens.first == "{" && parens.second == "}", "Incorrect spec for JustSortIndex");
    std::cout << "Building DummyIndex" << std::endl;
    return std::make_unique<DummyIndex<D>>();
}

template <size_t D>
std::unique_ptr<JustSortIndex<D>> IndexBuilder<D>::BuildJustSortIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    size_t dim;
    spec >> parens.first >> dim >> parens.second;
    AssertWithMessage(parens.first == "{" && parens.second == "}", "Incorrect spec for JustSortIndex");
    std::cout << "Building JustSortIndex on dim " << dim << std::endl;
    return std::make_unique<JustSortIndex<D>>(dim);
}

template <size_t D>
std::unique_ptr<SecondaryBTreeIndex<D>> IndexBuilder<D>::BuildSecondaryBTreeIndex(std::ifstream& spec) {
    std::string paren, optional_list, paren2;
    size_t dim;
    spec >> paren >> dim >> optional_list;
    auto index = std::make_unique<SecondaryBTreeIndex<D>>(dim);
    if (optional_list == "}") {
        AssertWithMessage(paren == "{", "Incorrect spec for SecondaryBTreeIndex");
        return index;
        std::cout << "Building SecondaryBTreeIndex with dim " << dim << std::endl;
    }
    spec >> paren2;
    AssertWithMessage(paren == "{" && paren2 == "}", "Incorrect spec for SecondaryBTreeIndex");
    List<Key> outlier_list = load_binary_file<Key>(optional_list);
    std::cout << "Building SecondaryBTreeIndex with dim " << dim << " and outlier list of size " << outlier_list.size() << std::endl;
    index->SetIndexList(outlier_list);
    return index;
}

template <size_t D>
std::unique_ptr<BinarySearchIndex<D>> IndexBuilder<D>::BuildBinarySearchIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    std::string opt_file;
    size_t dim;
    spec >> parens.first >> dim >> opt_file;
    if (opt_file == "}") {
        AssertWithMessage(parens.first == "{", "Incorrect spec for BinarySearchIndex");
        std::cout << "Building BinarySearchIndex on dim " << dim << std::endl;
        return std::make_unique<BinarySearchIndex<D>>(dim);
    } else {
        spec >> parens.second;
        std::cout << "Building BinarySearchIndex with targets file on dim " << dim << std::endl;
        return std::make_unique<BinarySearchIndex<D>>(dim, opt_file);
    }
}

template <size_t D>
std::unique_ptr<PrimaryBTreeIndex<D>> IndexBuilder<D>::BuildPrimaryBTreeIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    size_t dim, page_size;
    spec >> parens.first >> dim >> page_size >> parens.second;
    AssertWithMessage(parens.first == "{" && parens.second == "}", "Incorrect spec for PrimaryBTreeIndex");
    std::cout << "Building PrimaryBTreeIndex with dim " << dim << " and page size " << page_size << std::endl;
    return std::make_unique<PrimaryBTreeIndex<D>>(dim, page_size);
}

template <size_t D>
std::unique_ptr<MappedCorrelationIndex<D>> IndexBuilder<D>::BuildMappedCorrelationIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    std::string mapping_file, target_bucket_file;
    spec >> parens.first >> mapping_file >> target_bucket_file >> parens.second;
    AssertWithMessage(parens.first == "{" && parens.second == "}", "Incorrect spec for MappedCorrelationIndex");
    std::cout << "Building MappedCorrelationIndex" << std::endl;
    return std::make_unique<MappedCorrelationIndex<D>>(mapping_file, target_bucket_file);
}

template <size_t D>
std::unique_ptr<CombinedCorrelationIndex<D>> IndexBuilder<D>::BuildCombinedCorrelationIndex(std::ifstream& spec) {
    std::pair<std::string, std::string> parens;
    spec >> parens.first;
    auto map_index = Dispatch(spec);
    auto sec_index = Dispatch(spec);
    spec >> parens.second;
    AssertWithMessage(parens.first == "{" && parens.second == "}", "Incorrect spec for CombinedCorrelationIndex");
    auto comb_index = std::make_unique<CombinedCorrelationIndex<D>>();
    comb_index->SetMappedIndex(std::unique_ptr<MappedCorrelationIndex<D>>(
                dynamic_cast<MappedCorrelationIndex<D>*>(map_index.release())));
    std::cout << "Adding MappedCorrelationIndex to CombinedCorrelationIndex" << std::endl;
    comb_index->SetOutlierIndex(std::unique_ptr<SecondaryIndexer<D>>(
                dynamic_cast<SecondaryIndexer<D>*>(sec_index.release())));
    std::cout << "Adding SecondaryIndex to CombinedCorrelationIndex" << std::endl;
    std::cout << "Building CombinedCorrelationIndex" << std::endl;
    return comb_index;
}

template <size_t D>
std::unique_ptr<CompositeIndex<D>> IndexBuilder<D>::BuildCompositeIndex(std::ifstream& spec) {
    std::string paren1;
    spec >> paren1;
    AssertWithMessage(paren1 == "{", "Incorrect spec for CompositeIndex");
    auto composite_index = std::make_unique<CompositeIndex<D>>(0);
    while (true) {
        auto next_index = Dispatch(spec);
        if (!next_index) {
            break;
        }
        switch (next_index->Type()) {
            case Primary:
                composite_index->SetPrimaryIndex(std::unique_ptr<PrimaryIndexer<D>>(
                            static_cast<PrimaryIndexer<D>*>(next_index.release())));
                std::cout << "Setting primary index for CompositeIndex" << std::endl;
                break;
            case Secondary:
                composite_index->AddSecondaryIndex(std::unique_ptr<SecondaryIndexer<D>>(
                            static_cast<SecondaryIndexer<D>*>(next_index.release())));
                std::cout << "Adding secondary index to CompositeIndex" << std::endl;
                break;
            case Correlation:
                composite_index->AddCorrelationIndex(std::unique_ptr<CorrelationIndexer<D>>(
                            static_cast<CorrelationIndexer<D>*>(next_index.release())));
                std::cout << "Adding correlation index to CompositeIndex" << std::endl;
                break;
            case Rewriting:
                composite_index->AddRewriter(std::unique_ptr<Rewriter<D>>(
                            static_cast<Rewriter<D>*>(next_index.release())));
                std::cout << "Adding rewriter to CompositeIndex" << std::endl;
                break;
            default:
                AssertWithMessage(false, "Unrecognized index type " + std::to_string(next_index->Type()));
        }
    }
    std::cout << "Building CompositeIndex" << std::endl;
    return composite_index;
} 

template <size_t D>
std::unique_ptr<OctreeIndex<D>> IndexBuilder<D>::BuildOctreeIndex(std::ifstream& spec) {
    std::string token;
    spec >> token;
    AssertWithMessage(token == "{", "Incorrect spec for OctreeIndex");
    std::vector<std::string> params;
    while (spec >> token) {
        if (token == "}") {
            break;
        }
        params.push_back(token);
    }
    AssertWithMessage(params.size() > 1, "Octree requires page_size and at least one indexed dimension");
    size_t page_size = std::stoi(params[0]);
    std::vector<size_t> indexed_dims;
    for (size_t i = 1; i < params.size(); i++) {
        indexed_dims.push_back(std::stoi(params[i]));
    }
    std::cout << "Building Octree index with page size " << page_size << " and "
        << indexed_dims.size() << " columns" << std::endl;
    return std::make_unique<OctreeIndex<D>>(indexed_dims, page_size);
}

template <size_t D>
std::unique_ptr<FloodIndex<D>> IndexBuilder<D>::BuildFloodIndex(std::ifstream& spec) {
    std::string token;
    spec >> token;
    AssertWithMessage(token == "{", "Incorrect spec for FloodIndex");
    std::string dirname;
    spec >> dirname;
    spec >> token;
    AssertWithMessage(token == "}", "Incorret spec for FloodIndex");
    return std::make_unique<FloodIndex<D>>(dirname);
}

template <size_t D>
std::unique_ptr<BucketedSecondaryIndex<D>> IndexBuilder<D>::BuildBucketedSecondaryIndex(std::ifstream& spec) {
    std::string paren, mapfile, optional_list, paren2;
    size_t dim;
    spec >> paren >> dim >> mapfile >> optional_list;
    if (optional_list == "}") {
        AssertWithMessage(paren == "{", "Incorrect spec for BucketedSecondaryIndex");
        auto idx = std::make_unique<BucketedSecondaryIndex<D>>(dim);
        idx->SetBucketFile(mapfile);
        return idx;
        std::cout << "Building BucketedSecondaryIndex with dim " << dim << " and mapfile" << std::endl;
    }
    spec >> paren2;
    AssertWithMessage(paren == "{" && paren2 == "}", "Incorrect spec for BucketedSecondaryIndex");
    List<Key> outlier_list = load_binary_file<Key>(optional_list);
    std::cout << "Building BucketedSecondaryIndex with dim " << dim << ", mapfile"
        << ", and outlier list of size " << outlier_list.size() << std::endl;
    auto idx = std::make_unique<BucketedSecondaryIndex<D>>(dim, outlier_list);
    idx->SetBucketFile(mapfile);
    return idx;
}

template <size_t D>
std::unique_ptr<LinearModelRewriter<D>> IndexBuilder<D>::BuildLinearModelRewriter(std::ifstream& spec) {
    std::string paren, mapfile;
    spec >> paren >> mapfile;
    AssertWithMessage(paren == "{", "Incorrect spec for LinearModelRewriter");
    auto rewriter = std::make_unique<LinearModelRewriter<D>>(mapfile);
    
    auto next_index = Dispatch(spec);
    AssertWithMessage(next_index != nullptr, "Expected an auxiliary index for rewriter");
    AssertWithMessage(next_index->Type() == Secondary, "Expected Secondary Indexer for writer");
    rewriter->SetAuxiliaryIndex(std::unique_ptr<SecondaryBTreeIndex<D>>(
                        dynamic_cast<SecondaryBTreeIndex<D>*>(next_index.release())));
    std::cout << "Setting secondary index for LinearModelRewriter" << std::endl;
    spec >> paren;
    AssertWithMessage(paren == "}", "Incorrect spec: expected '}'");
    return rewriter;
}

template <size_t D>
std::unique_ptr<TRSTreeRewriter<D>> IndexBuilder<D>::BuildTRSTreeRewriter(std::ifstream& spec) {
    std::string paren;
    size_t mapped_dim, target_dim, err_bd;
    spec >> paren >> mapped_dim >> target_dim >> err_bd;
    AssertWithMessage(paren == "{", "Incorrect spec for TRSTreeRewriter");
    auto rewriter = std::make_unique<TRSTreeRewriter<D>>(mapped_dim, target_dim, err_bd);
    auto next_index = Dispatch(spec);
    AssertWithMessage(next_index != nullptr, "Expected an auxiliary index for rewriter");
    AssertWithMessage(next_index->Type() == Secondary, "Expected Secondary Indexer for writer");
    rewriter->SetAuxiliaryIndex(std::unique_ptr<SecondaryBTreeIndex<D>>(
                        dynamic_cast<SecondaryBTreeIndex<D>*>(next_index.release())));
    std::cout << "Setting secondary index for TRSTreeRewriter" << std::endl;
    spec >> paren;
    AssertWithMessage(paren == "}", "Incorrect spec: expected '}'");
    return rewriter;
}

template <size_t D>
std::unique_ptr<MeasureBetaIndex<D>> IndexBuilder<D>::BuildMeasureBetaIndex(std::ifstream& spec) {
    std::string paren;
    size_t dim;
    spec >> paren;
    AssertWithMessage(paren == "{", "Incorrect spec for MeasuredBetaIndex");
    auto mbi = std::make_unique<MeasureBetaIndex<D>>(dim);
    auto prim_index = Dispatch(spec);
    AssertWithMessage(prim_index != nullptr, "Expected a primary index for MeasureBeta index");
    AssertWithMessage(prim_index->Type() == Primary, "Expected Primary Indexer for MeasureBeta index");
    mbi->SetPrimaryIndex(std::unique_ptr<PrimaryIndexer<D>>(
                        dynamic_cast<PrimaryIndexer<D>*>(prim_index.release())));
    std::cout << "Setting primary index for MeasureBetaIndex" << std::endl;
    auto next_index = Dispatch(spec);
    AssertWithMessage(next_index != nullptr, "Expected an auxiliary index for MeasureBeta index");
    AssertWithMessage(next_index->Type() == Secondary, "Expected Secondary Indexer for MeasureBeta index");
    mbi->SetAuxiliaryIndex(std::unique_ptr<SecondaryIndexer<D>>(
                        dynamic_cast<SecondaryIndexer<D>*>(next_index.release())));
    std::cout << "Setting secondary index for MeasuredBetaIndex" << std::endl;
    spec >> paren;
    AssertWithMessage(paren == "}", "Incorrect spec: expected '}'");
    return mbi;
}


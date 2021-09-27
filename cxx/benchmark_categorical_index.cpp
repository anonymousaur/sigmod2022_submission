#include <iostream>
#include <algorithm>
#include <chrono>
#include <sysexits.h>
#include <vector>

#include "types.h"
#include "flags.h"
#include "outlier_index.h"
#include "composite_index.h"
#include "binary_search_index.h"
#include "primary_btree_index.h"
#include "dummy_index.h"
#include "secondary_btree_index.h"
#include "single_column_rewriter.h"
#include "row_order_dataset.h"
#include "inmemory_column_order_dataset.h"
#include "query_engine.h"
#include "visitor.h"
#include "utils.h"
#include "datacube.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Expected arguments: --dataset --workload --visitor "
            << "[--outlier-list] [--mapping] [--page-size] [--composite-gap]"
            << "[--save] [--results_folder]" << std::endl;
        return EX_USAGE;
    }
    auto flags = ParseFlags(argc, argv);

    cout << "Dimension is " << DIM << endl;

    std::string dataset_file = GetRequired(flags, "dataset");
    auto data = load_binary_file< Point<DIM> >(dataset_file);
    std::string workload_file = GetRequired(flags, "workload");
    vector< Query<DIM> > workload = load_query_file<DIM>(workload_file);
    std::string outlier_list_file = GetWithDefault(flags, "outlier-list", "");
    std::vector<size_t> outlier_list;
    if (!outlier_list_file.empty()) {
        outlier_list = load_binary_file<size_t>(outlier_list_file);
        std::cout << "Found " << outlier_list.size() << " outliers" << std::endl;
    }
    std::cout << "Loaded dataset and workload" << std::endl;

    int num_queries = std::stoi(GetWithDefault(flags, "num_queries", "100000"));
    num_queries = std::min(num_queries, (int)workload.size());

    // Define datacubes
    auto index_creation_start = std::chrono::high_resolution_clock::now();
    auto rewriter = std::make_unique<SingleColumnRewriter<DIM>>(GetRequired(flags, "mapping"));
    std::cout << "Loaded rewriter with mapped dimension " << rewriter->MappedDim()
        << " and target " << rewriter->TargetDim() << std::endl;
    
    int page_size = std::stoi(GetWithDefault(flags, "page-size", "1"));
    int composite_gap = std::stoi(GetWithDefault(flags, "composite-gap", "1"));
    // Outlier with primary index
    assert (outlier_list.size() > 0);
    auto indexer = std::make_unique<OutlierIndex<DIM>>(outlier_list);
    auto main_indexer = std::make_unique<CompositeIndex<DIM>>(composite_gap);
    main_indexer->SetPrimaryIndex(std::make_unique<BinarySearchIndex<DIM>>(rewriter->TargetDim()));
    main_indexer->AddSecondaryIndex(std::make_unique<SecondaryBTreeIndex<DIM>>(rewriter->MappedDim())); 
    indexer->SetIndexer(std::move(main_indexer));
    indexer->SetOutlierIndexer(std::make_unique<BinarySearchIndex<DIM>>(rewriter->MappedDim()));

    //indexer->SetIndexer(std::make_unique<PrimaryBTreeIndex<DIM>>(rewriter->TargetDim(), page_size));
    //indexer->SetOutlierIndexer(std::make_unique<PrimaryBTreeIndex<DIM>>(rewriter->MappedDim(), page_size));
    //indexer->SetIndexer(std::make_unique<BinarySearchIndex<DIM>>(rewriter->TargetDim()));

    // CM with primary index
    //auto indexer = std::make_unique<CompositeIndex<DIM>>(composite_gap);
    //indexer->SetPrimaryIndex(std::make_unique<PrimaryBTreeIndex<DIM>>(rewriter->TargetDim(), page_size));
    //indexer->SetPrimaryIndex(std::make_unique<BinarySearchIndex<DIM>>(rewriter->TargetDim()));
    //indexer->AddSecondaryIndex(std::make_unique<SecondaryBTreeIndex<DIM>>(rewriter->MappedDim())); 
    
    // Full scan
    //auto indexer = std::make_unique<DummyIndex<DIM>>();

    std::cout << "Sorting data..." << std::endl;   
    auto before_sort_time = std::chrono::high_resolution_clock::now();
    indexer->Init(data.begin(), data.end());
    auto after_sort_time = std::chrono::high_resolution_clock::now();
    auto tt_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(after_sort_time - before_sort_time).count();
    std::cout << "Time to sort and finalize data: " << tt_sort / 1e9 << "s" << std::endl;

    auto compression_start = std::chrono::high_resolution_clock::now();
    std::vector<Datacube<DIM>> cubes;
    std::cout << "Not using datacubes" << std::endl;
    bool use_datacubes = false;

    auto dataset = std::make_unique<RowOrderDataset<DIM>>(data);
    //auto dataset = std::unique_ptr<InMemoryColumnOrderDataset<DIM>>(data);
    auto compression_finish = std::chrono::high_resolution_clock::now();
    auto compression_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compression_finish-compression_start).count();
    std::cout << "Compression time: " << compression_time / 1e9 << "s" << std::endl;
    std::cout << "Indexer sizei (B): " << indexer->Size() << std::endl;

    QueryEngine<DIM> engine(std::move(dataset), std::move(indexer));
    auto index_creation_finish = std::chrono::high_resolution_clock::now();
    auto index_creation_time = std::chrono::duration_cast<std::chrono::nanoseconds>(index_creation_finish-index_creation_start).count();
    std::cout << "Index creation time: " << index_creation_time / 1e9 << "s" << std::endl;

    // cout << "Index size: " << index.size() << " bytes" << std::endl;

    cout << "Starting queries..." << endl;
    size_t total = 0;
    Scalar aggregate = 0;
    std::vector<double> query_times;
    query_times.reserve(num_queries);
    std::string visitor_type = std::string(GetRequired(flags, "visitor"));
    std::string timeout_str = GetWithDefault(flags, "timeout", "no");
    bool timeout = timeout_str == "yes";
    auto start = std::chrono::high_resolution_clock::now();
    if (visitor_type == std::string("collect")) {
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            CollectVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            total += visitor.result_set.size();
            auto query_end = std::chrono::high_resolution_clock::now();
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
        }
    } else if (visitor_type == std::string("count")) {
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            CountVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            total += visitor.count;
            auto query_end = std::chrono::high_resolution_clock::now();
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
        }
    } else if (visitor_type == std::string("index")) {
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            IndexVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            total += visitor.indexes.size();
            auto query_end = std::chrono::high_resolution_clock::now();
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
        }
    } else if (visitor_type == std::string("dummy")) {
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            DummyVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            auto query_end = std::chrono::high_resolution_clock::now();
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
        }
    } else if (visitor_type == std::string("sum")) {
       for (int i = 0; i < num_queries; i++) {
           Query<DIM> q = workload[i];
           auto query_start = std::chrono::high_resolution_clock::now();
           SumVisitor<DIM> visitor(0);
           engine.Execute(q, visitor);
           aggregate += visitor.sum;
           auto query_end = std::chrono::high_resolution_clock::now();
           query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
           if (timeout && i % 10 == 0) {
               if (std::chrono::duration_cast<std::chrono::seconds>(query_end - start).count() > 60) {
                   num_queries = i;
                   break;
               }
           }
       }
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto tt = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
    if (visitor_type == std::string("aggregate")) {
        std::cout << "Total aggregate returned by queries: " << aggregate << std::endl;
    } else if (visitor_type == "sum") {
        std::cout << "Sum returned by queries: " << aggregate << std::endl;
    } else {
        std::cout << "Total points returned by queries: " << total << std::endl;
    }
    std::cout << "Total queries: " << num_queries << std::endl;
    std::cout << "Avg range query time (ns): " << tt / ((double) num_queries) << std::endl;
    
    return 0;
}

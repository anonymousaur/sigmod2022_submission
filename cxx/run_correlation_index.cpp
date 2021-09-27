#include <iostream>
#include <algorithm>
#include <chrono>
#include <sysexits.h>
#include <vector>

#include "types.h"
#include "flags.h"
#include "index_builder.h"
#include "row_order_dataset.h"
#include "inmemory_column_order_dataset.h"
#include "compressed_column_order_dataset.h"
#include "clustered_column_order_dataset.h"
#include "query_engine.h"
#include "visitor.h"
#include "utils.h"
#include "datacube.h"

using namespace std;


void run_workload(QueryEngine<DIM>& engine,
        const std::string& visitor_type,
        const std::string& workload_file,
        std::ofstream& savefile) {

    std::vector<Query<DIM>> workload = load_query_file<DIM>(workload_file);
    size_t num_queries = workload.size();
    cout << endl << "Starting workload " << workload_file << "(" << num_queries << " queries)" << endl;

    size_t total = 0;
    Scalar aggregate = 0;
    long long points_scanned = 0;
    std::vector<double> query_times;
    query_times.reserve(num_queries);
    std::vector<size_t> results;
    results.reserve(num_queries);
    auto start = std::chrono::high_resolution_clock::now();
    if (visitor_type == std::string("collect")) {
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            CollectVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            total += visitor.result_set.size();
            results.push_back(visitor.result_set.size());
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
            results.push_back(visitor.count);
            auto query_end = std::chrono::high_resolution_clock::now();
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
        }
    } else if (visitor_type == std::string("index")) {
        long prev_r = 0;
        long prev_l = 0;
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            IndexVisitor<DIM> visitor;
            engine.Execute(q, visitor);
            total += visitor.indexes.size();
            auto query_end = std::chrono::high_resolution_clock::now();
            results.push_back(visitor.indexes.size());
            long tot_r_scanned = engine.ScannedRangePoints();
            long tot_l_scanned = engine.ScannedListPoints();
            std::cout << "True: " << visitor.indexes.size()
                << ", scanned range = " << tot_r_scanned - prev_r
                << ", scanned list = " << tot_l_scanned - prev_l
                << std::endl;
            prev_r = tot_r_scanned;
            prev_l = tot_l_scanned;
            auto query_t = std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count();
            query_times.push_back(query_t);
            std::cout << "Query " << i << " time (ms): " << query_t/ 1e6 << std::endl;
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
        long prev = 0;
        for (int i = 0; i < num_queries; i++) {
            Query<DIM> q = workload[i];
            auto query_start = std::chrono::high_resolution_clock::now();
            SumVisitor<DIM> visitor(1);
            engine.Execute(q, visitor);
            aggregate += visitor.sum;
            results.push_back(visitor.sum);
            auto query_end = std::chrono::high_resolution_clock::now();
            long tot_scanned = engine.ScannedPoints();
            prev = tot_scanned;
            query_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(query_end-query_start).count());
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
    std::cout << "Total points scanned: " << engine.ScannedRangePoints() << " (range), "
        << engine.ScannedListPoints() << " (list)" << std::endl;    

    savefile << "workload: " << workload_file << std::endl
            << "num_queries: " << num_queries << std::endl
            << "visitor: " << visitor_type << std::endl
            << "total_pts: " << total << std::endl
            << "aggregate: " << aggregate << std::endl;
    engine.WriteStats(savefile);
    savefile << "avg_query_time_ns: " << (tt/(double)num_queries) << std::endl;
    engine.Reset();
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Expected arguments: --dataset --workload --visitor "
            << "--indexer-spec --save [--base-cols]" << std::endl;
        return EX_USAGE;
    }
    auto flags = ParseFlags(argc, argv);

    cout << "Dimension is " << DIM << endl;

    std::string dataset_file = GetRequired(flags, "dataset");
    std::vector<Point<DIM>> data;
    size_t base_cols = std::stoi(GetWithDefault(flags, "base-cols", "0"));
    if (base_cols > 0) {
        data = load_binary_dataset_with_repl<DIM>(dataset_file, base_cols);
    } else {
        data = load_binary_file< Point<DIM> >(dataset_file);
    }
    vector<string> workload_files = GetCommaSeparated(flags, "workload");
    std::cout << "Loaded dataset and workload" << std::endl;
    string save_file_base = GetWithDefault(flags, "save", "");

    // Define datacubes
    auto index_creation_start = std::chrono::high_resolution_clock::now();
    
    size_t num_outliers = 0;

    IndexBuilder<DIM> ix_builder;
    auto indexer = ix_builder.Build(GetRequired(flags, "indexer-spec"));

    std::cout << "Sorting data..." << std::endl;   
    auto before_sort_time = std::chrono::high_resolution_clock::now();
    if (indexer->Init(data.begin(), data.end())) {
        std::cout << "Data was modified. Aborting..." << std::endl;
        return 0;
    }
    auto after_sort_time = std::chrono::high_resolution_clock::now();
    auto tt_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(after_sort_time - before_sort_time).count();
    std::cout << "Time to sort and finalize data: " << tt_sort / 1e9 << "s" << std::endl;

    auto compression_start = std::chrono::high_resolution_clock::now();
    std::vector<Datacube<DIM>> cubes;
    std::cout << "Not using datacubes" << std::endl;
    bool use_datacubes = false;

    auto dataset = std::make_shared<CompressedColumnOrderDataset<DIM>>(data);
    //auto dataset = std::make_shared<ClusteredColumnOrderDataset<DIM>>(data);
    indexer->SetDataset(dataset);
    auto compression_finish = std::chrono::high_resolution_clock::now();
    auto compression_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compression_finish-compression_start).count();
    std::cout << "Compression time: " << compression_time / 1e9 << "s" << std::endl;
    size_t indexer_size_bytes = indexer->Size();
    std::cout << "Indexer sizei (B): " << indexer_size_bytes << std::endl;

    QueryEngine<DIM> engine(dataset, indexer);
    auto index_creation_finish = std::chrono::high_resolution_clock::now();
    auto index_creation_time = std::chrono::duration_cast<std::chrono::nanoseconds>(index_creation_finish-index_creation_start).count();
    std::cout << "Index creation time: " << index_creation_time / 1e9 << "s" << std::endl;


    string visitor_type = std::string(GetRequired(flags, "visitor"));
    for (size_t work_ix = 0; work_ix < workload_files.size(); work_ix++) {
        auto cur_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        std::ofstream results;
        if (!save_file_base.empty()) {
            string filename = save_file_base + "_" + std::to_string(cur_timestamp);
            results.open(filename);
        } else {
            results.open("default.out");
        }
        assert (results.is_open());
        results << "timestamp: " << cur_timestamp << std::endl
            << "name: " << GetWithDefault(flags, "name", "") << std::endl
            << "dataset: " << GetRequired(flags, "dataset") << std::endl
            << "index_spec: " << GetRequired(flags, "indexer-spec") << std::endl
            << "index_size: " << indexer->Size() << std::endl;
        run_workload(engine, visitor_type, workload_files[work_ix], results); 
        results.close();
        cout << endl << "==========================" << endl;
    }
}

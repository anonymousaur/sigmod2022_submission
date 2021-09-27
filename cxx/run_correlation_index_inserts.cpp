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


int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Expected arguments: --name --dataset --insert-batch-size"
            << "--indexer-spec --save [--base-cols]" << std::endl;
        return EX_USAGE;
    }
    auto flags = ParseFlags(argc, argv);

    cout << "Dimension is " << DIM << endl;

    std::string name = GetRequired(flags, "name");
    std::string dataset_file = GetRequired(flags, "dataset");
    std::vector<Point<DIM>> data;
    size_t base_cols = std::stoi(GetWithDefault(flags, "base-cols", "0"));
    if (base_cols > 0) {
        data = load_binary_dataset_with_repl<DIM>(dataset_file, base_cols);
    } else {
        data = load_binary_file< Point<DIM> >(dataset_file);
    }
    string save_file_base = GetWithDefault(flags, "save", "");

    // Define datacubes
    auto index_creation_start = std::chrono::high_resolution_clock::now();
    
    size_t num_outliers = 0;
    std::cout << "Building index" << std::endl;
    IndexBuilder<DIM> ix_builder;
    auto indexer = ix_builder.Build(GetRequired(flags, "indexer-spec"));
    size_t batch_size = std::stoi(GetRequired(flags, "insert-batch-size"));
    size_t initial_size = std::stoi(GetWithDefault(flags, "initial-size", "0"));

    std::cout << "Sorting data..." << std::endl;   
    auto before_sort_time = std::chrono::high_resolution_clock::now();
    indexer->Init(data.begin(), data.begin());
    auto after_sort_time = std::chrono::high_resolution_clock::now();
    auto tt_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(after_sort_time - before_sort_time).count();
    std::cout << "Time to sort and finalize data: " << tt_sort / 1e9 << "s" << std::endl;

    auto compression_start = std::chrono::high_resolution_clock::now();
    //auto dataset = std::make_shared<CompressedColumnOrderDataset<DIM>>(data);
    auto dataset = std::make_shared<ClusteredColumnOrderDataset<DIM>>(data);
    indexer->SetDataset(dataset);
    
    auto compression_finish = std::chrono::high_resolution_clock::now();
    auto compression_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compression_finish-compression_start).count();
    std::cout << "Compression time: " << compression_time / 1e9 << "s" << std::endl;
    size_t indexer_size_bytes = indexer->Size();
    std::cout << "Indexer sizei (B): " << indexer_size_bytes << std::endl;

    std::vector<Point<DIM>> batch(data.begin(), data.begin() + initial_size);
    std::vector<size_t> indexes(batch.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    auto insert_start = std::chrono::high_resolution_clock::now();
    indexer->DummyInsert(batch, indexes);
    auto insert_finish = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::nanoseconds>(insert_finish - insert_start).count();
    std::cout << "Insertion time = (size = " << initial_size << ") = " << insert_time /1e6 << "ms" << std::endl;

    std::vector<long> update_times;    
    for (size_t s = initial_size; s + batch_size <= dataset->Size(); s += batch_size) {
        std::vector<Point<DIM>> update_batch(data.begin() + s, data.begin() + s + batch_size);
        std::vector<size_t> update_indexes(update_batch.size());
        std::iota(update_indexes.begin(), update_indexes.end(), s);
        auto update_start = std::chrono::high_resolution_clock::now();
        indexer->DummyInsert(update_batch, update_indexes);
        auto update_finish = std::chrono::high_resolution_clock::now();
        auto update_time = std::chrono::duration_cast<std::chrono::nanoseconds>(update_finish - update_start).count();
        std::cout << "Update time (total size = " << s + batch_size << ") = " << update_time /1e6 << "ms" << std::endl;
        update_times.push_back(update_time);
    }
    std::cout << "Index size: " << indexer->Size() << std::endl;    

    std::string savefile = GetWithDefault(flags, "save", "");
    if (!savefile.empty()) {
        auto f = std::ofstream(savefile);
        auto cur_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        f << "timestamp: " << cur_timestamp << std::endl
            << "name: " << name << std::endl
            << "dataset: " << GetRequired(flags, "dataset") << std::endl
            << "total_size: " << dataset->Size() << std::endl
            << "index_spec: " << GetRequired(flags, "indexer-spec") << std::endl
            << "insert_size: " << initial_size << std::endl
            << "update_size: " << batch_size << std::endl
            << "index_size: " << indexer->Size() << std::endl
            << "insert_time_ms: " << insert_time / 1e6 << std::endl;
        if (update_times.size() > 0) {
            float avg_qt = 0;         
            for (long qt : update_times) {
                avg_qt += float(qt) / 1e6;
            }
            f << "average_update_time_ms: " << avg_qt / update_times.size() << std::endl;
        }
        f.close();
    }
    
    // Just to make sure everything isn't optimized away.
    //Query<DIM> q;
    //auto r = indexer->IndexRanges(q);
    //if (r.ranges.size() > 0) {
    //    std::cout << "Ranges: " << r.ranges[0].start << std::endl;
    //}

}

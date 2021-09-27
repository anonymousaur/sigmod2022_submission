/*
 * Benchmark for sorting algorithms on partially sorted data (e.g. merging multiple sorted lists
 * together)
 */

#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#include "timsort.hpp"
#include "merge_utils.h"


int main() {
    const size_t NVECS = 50;
    
    std::default_random_engine gen;
    std::uniform_int_distribution<size_t> dist(1,1<<30);
    
    std::vector<std::vector<size_t> *> vecs;
    for (size_t i = 0; i < NVECS; i++) {
        auto v = new std::vector<size_t>();
        v->reserve(200000);
        for (size_t j = 0; j < 200000; j++) {
            v->push_back(dist(gen));
        }
        std::sort(v->begin(), v->end());
        vecs.push_back(v);
    }
    
    std::vector<size_t> full;
    for (size_t i = 0; i < NVECS; i++) {
        full.insert(full.end(), vecs[i]->begin(), vecs[i]->end());
    }
    
    auto start_sort = std::chrono::high_resolution_clock::now();
    std::sort(full.begin(), full.end());
    auto end_sort = std::chrono::high_resolution_clock::now();
    std::cout << full[full.size()/2] << std::endl;
    auto tt = std::chrono::duration_cast<std::chrono::nanoseconds>(end_sort-start_sort).count();
    std::cout << "std::sort time: " << tt / 1e3 << "us" << std::endl;
    
    
    std::vector<size_t> full2;
    for (size_t i = 0; i < NVECS; i++) {
        full2.insert(full2.end(), vecs[i]->begin(), vecs[i]->end());
    }
    auto start_timsort = std::chrono::high_resolution_clock::now();
    gfx::timsort(full2.begin(), full2.end());
    auto end_timsort = std::chrono::high_resolution_clock::now();
    std::cout << full2[full2.size()/2] << std::endl;
    tt = std::chrono::duration_cast<std::chrono::nanoseconds>(end_timsort-start_timsort).count();
    std::cout << "std::sort time: " << tt / 1e3 << "us" << std::endl;
    
    
    auto start_merge = std::chrono::high_resolution_clock::now();
    auto merge_vec = MergeUtils::Union(vecs);
    auto end_merge = std::chrono::high_resolution_clock::now();
    std::cout << merge_vec[merge_vec.size()/2] << std::endl;
    tt = std::chrono::duration_cast<std::chrono::nanoseconds>(end_merge-start_merge).count();
    std::cout << "std::sort time: " << tt / 1e3 << "us" << std::endl;

    return 0;
}


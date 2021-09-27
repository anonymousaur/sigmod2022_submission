/**
 * A column-ordered dataset, which stores a single coordinate for each point continguously.
 * Each coordinate is stored in a separate data structure.
 */

#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cassert>

#include "dataset.h"
#include "types.h"
#include "disk_storage_manager.h"
#include "buffer_manager.h"

template <size_t D>
class ColumnOrderDataset : public Dataset<D> {
  
  public:
    explicit ColumnOrderDataset(std::vector<Point<D>> data) {
        for (const Point<D> &p : data) {
            for (size_t d = 0; d < D; d++) {
                columns_[d].push_back(p[d]);
            }
        }
    }

    ~ColumnOrderDataset() {
        delete dsm_;
        delete bm_;
    }

    Point<D> Get(size_t i) const override {
        Point<D> data;
        for (size_t d = 0; d < D; d++) {
            data[d] = GetCoord(i, d);
        }
        return data;
    }

    /**
     * Get coordinate `dim` of point at index `index`
     */
    Scalar GetCoord(size_t index, size_t dim) const override {
        if (on_disk_) {
            size_t page_in_col = (size_t)(std::upper_bound(page_boundaries_.begin(), page_boundaries_.end(), index) -
                    page_boundaries_.begin() - 1);
            size_t logical_page = pages_per_column_ * dim + page_in_col;
            id_type page_id = page_mapping_.at(logical_page);
            assert(page_id != DiskStorageManager::EmptyPage);
            Scalar *data = (Scalar *)(bm_->loadPage(page_id));
            size_t page_start_idx = page_boundaries_[page_in_col];
            return data[index - page_start_idx];
        } else {
            return columns_[dim][index];
        }
    }

    size_t Size() const override {
        return columns_[0].size();
    }

    size_t NumDims() const override {
        return D;
    }

    size_t PrimaryKeyColumn() const override {
        return 0;
    }

    size_t SizeInBytes() const override {
        return Size() * NumDims();
    }

    // Disk storage
    DiskStorageManager *dsm_ = nullptr;
    BufferManager *bm_ = nullptr;
    size_t pages_per_column_;
    bool on_disk_ = false;
    std::vector<size_t> page_boundaries_;
    std::unordered_map<size_t, id_type> page_mapping_;

    // page_boundaries should contain the start of the first page (i.e., 0) and the end of the last page (i.e., the size)
    void SaveToDisk(const std::vector<size_t>& page_boundaries, std::string filename, uint32_t page_size) {
        dsm_ = new DiskStorageManager(filename, page_size);
        pages_per_column_ = page_boundaries.size() - 1;
        on_disk_ = true;
        page_boundaries_ = page_boundaries;
        size_t logical_page = 0;
        for (size_t d = 0; d < D; d++) {
            size_t page_start = 0;
            size_t page_end;  // exclusive
            for (size_t i = 1; i < page_boundaries.size(); i++) {
                page_end = page_boundaries[i];
                SavePageToDisk(d, page_start, page_end, logical_page);
                logical_page++;
                page_start = page_end;
            }
        }
        dsm_->flush();

        // Remove dataset from memory
        for (size_t d = 0; d < D; d++) {
            std::vector<Scalar>().swap(columns_[d]);
        }

        bm_ = new BufferManager(dsm_);
    }

    void SavePageToDisk(size_t column, size_t start_idx, size_t end_idx, size_t logical_page) {
        if (end_idx == start_idx) {
            page_mapping_[logical_page] = DiskStorageManager::EmptyPage;
            return;
        }

        id_type page_id = DiskStorageManager::NewPage;
        dsm_->storeByteArray(
                page_id,
                (end_idx - start_idx) * sizeof(Scalar),
                (uint8_t *)(columns_[column].data() + start_idx)
        );
        page_mapping_[logical_page] = page_id;
    }

    void SetBufferMemoryLimit(double memory_limit) {
        bm_->setMemoryLimit(memory_limit);
    };

public:
    std::array<std::vector<Scalar>, D> columns_;
};


/**
 * A compressed column-ordered dataset, which stores a single coordinate for each point
 * continguously in a compressed delta-encoded scheme. The encoding scheme has checkpoints
 * at fixed intervals, so to decode, one finds the appropriate checkpoint in constant time,
 * and then access the correct bit offsets to find the delta. A typical random access takes O(1)
 * time. There are optimizations to speed up iterations as opposed to random accesses.
 */

#include <iostream>
#include <vector>

#include "datacube.h"
#include "dataset.h"
#include "types.h"
#include "cpp-btree/btree_map.h"

#pragma once

typedef unsigned __int128 uint128_t;

// There are 2^7 = 128 values per compression block.
// The block size should always be a power of 2 for fast computation.
const char COLUMN_COMPRESSION_BLOCK_SIZE_POW = 9;
const uint64_t COLUMN_COMPRESSION_BLOCK_SIZE_MASK = (1 << COLUMN_COMPRESSION_BLOCK_SIZE_POW) - 1;

// Each compression block stores metadata for a fixed number of
// entries, assuming that they all fit into 32 bits.
struct CompressionBlock {
    // The offset from the start of the data bitstream for this column where this block's data
    // begins.
    uint64_t bit_offset;
    // A mask that is `bit_width' bits wide.
    uint64_t bit_mask;
    // The minimum value in the block.
    Scalar base_value;
    // The number of bits each delta is decoded to.
    char bit_width;
    CompressionBlock() = default;
};

// For decoding sequential values, we save intermediate state here.
struct DecoderState {
    uint64_t byte_block;
    uint64_t mask;
    size_t shift;
    size_t cblk_ix;
    // We only use the base_value and bit_width fields from here.
    CompressionBlock cblk;
};

template <size_t D>
class CompressedColumnOrderDataset : public Dataset<D> {
  
  public:
    explicit CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
            const std::vector<Datacube<D>>& datacubes);
    explicit CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
                                          const std::vector<Datacube<D>>& datacubes, bool data_only);
    explicit CompressedColumnOrderDataset(const std::vector<Point<D>>& data, bool data_only);
    explicit CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
            const std::vector<Scalar>& row_ids=std::vector<Scalar>(),
            const std::vector<Datacube<D>>& datacubes=std::vector<Datacube<D>>());
    ~CompressedColumnOrderDataset();

    // Access methods: Get the full point from all columns.
    Point<D> Get(size_t i) const override;
    // Get a single coordinate of a single point. This function
    // must be extremely well optimized to allow fast scans.
    // TODO(vikram): Create a version of this function for sequential scans,
    // which stores intermediate state.
    Scalar GetCoord(size_t index, size_t dim) const override;



    Set<PhysicalIndex> Lookup(Set<Key> keys) override;

    // Optimize sequential scans over the dataset by storing intermediate
    // decoder state. This function may be slower for out of order scans since it incurs cost to
    // store decoder state, which is amortized only if subsequent calls are to immediately
    // following indices.
    uint64_t GetCoordRange(size_t start, size_t end, size_t dim, Scalar lower, Scalar upper) const override;
    void GetRangeValues(size_t start, size_t end, size_t dim, uint64_t valids, std::vector<Scalar> *results) const override;
    Scalar GetRangeSum(size_t start, size_t end, size_t dim, uint64_t valids) const override;

    size_t Size() const override {
        return size_;
    }

    size_t NumDims() const override {
        return D;
    }

    size_t PrimaryKeyColumn() const {
        return primary_key_column_;
    }

    uint64_t SizeInBytes() const override {
        uint64_t data_size = 0;
        for (size_t i = 0; i < compressed_column_sizes_.size(); i++) {
            data_size += compressed_column_sizes_[i];
        }
        uint64_t cblocks_size = (uint64_t)sizeof(CompressionBlock) * num_columns_ * blocks_per_column_;
        return data_size + cblocks_size;
    }

    // Public only for testing reasons.
    CompressionBlock *cblocks_;

private:
    Range<PhysicalIndex> LookupRange(Range<Key> range);
    // Compress the entire dataset.
    void Compress(const std::vector<std::vector<Scalar>>& columns);
    // Compress one particular column.
    void CompressColumn(size_t dim_idx, const std::vector<Scalar>& column);
    // Compress one particular compression block within a column, assuming the current offset is
    // `offset` from the start of the array. Add the resulting compression
    // block to `cblocks` and the resulting compressed bits to `data_bits`.
    // Return the offset of the bit following the last one added (this is where the next value would
    // go). 
    uint64_t CompressBlock(const std::vector<Scalar>& block, std::vector<CompressionBlock>* cblocks, std::vector<char>* data_bits, uint64_t offset);
    
    
    // Since all columns use the same number of cblocks, we can represent all of cblocks_ as a
    // single array. cblocks_[dim][ix] = cblocks_[dim * size_ + ix].
    char **column_data_;

    std::vector<Datacube<D>> cubes_;
    // Number of items in every column
    size_t size_;
    // // Number of total columns (data + cubes)
    size_t num_columns_;
    size_t blocks_per_column_;
    // Size of each column in bytes after compression
    std::vector<uint64_t> compressed_column_sizes_;
    const size_t primary_key_column_ = D;
    btree::btree_map<Key, PhysicalIndex> clustered_index_;
};

#include "../src/compressed_column_order_dataset.hpp"

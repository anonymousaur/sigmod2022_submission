#include "compressed_column_order_dataset.h"
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "utils.h"


template <size_t D>
CompressedColumnOrderDataset<D>::CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
        const std::vector<Scalar>& row_ids,
        const std::vector<Datacube<D>>& cubes) : clustered_index_() {
    size_ = data.size();
    std::vector<std::vector<Scalar>> columns(D + 1 + cubes.size());
    cubes_ = cubes;
    AssertWithMessage(cubes_.size() == 0, "Dataset does not support cubes");
    // Index the datacubes by the columns they will appear in.
    for (size_t i = 0; i < cubes_.size(); i++) {
        cubes_[i].index = D+1+i;
    }
    Key key = 0;
    PhysicalIndex ix = 0; 
    for (size_t i = 0; i < data.size(); i++) {
        const Point<D> &p = data[i];
        for (size_t d = 0; d < D; d++) {
            columns[d].push_back(p[d]);
        }
        columns[D].push_back(key);
        clustered_index_.insert(std::make_pair(key, ix));
        key++;
        ix++;
    }
    // Easier book-keeping.
    clustered_index_.insert(std::make_pair(key, ix));

    num_columns_ = D + 1 + cubes.size();
    blocks_per_column_ = (size_ + COLUMN_COMPRESSION_BLOCK_SIZE_MASK) >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;
        
    cblocks_ = (CompressionBlock *)malloc(sizeof(CompressionBlock) * num_columns_ * blocks_per_column_);
    column_data_ = (char **)malloc(sizeof(char *) * num_columns_);
    compressed_column_sizes_.resize(num_columns_);

    Compress(columns);
    std::cout << "Dataset size: " << SizeInBytes() << " bytes" << std::endl;
}

// A hacky way to make a compressed dataset without primary key
template <size_t D>
CompressedColumnOrderDataset<D>::CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
                                                              const std::vector<Datacube<D>>& cubes,
                                                              bool data_only) {
    assert(data_only);
    size_ = data.size();
    std::vector<std::vector<Scalar>> columns(D + cubes.size());
    cubes_ = cubes;
    // Index the datacubes by the columns they will appear in.
    for (size_t i = 0; i < cubes_.size(); i++) {
        cubes_[i].index = D+i;
    }
    for (size_t i = 0; i < data.size(); i++) {
        const Point<D> &p = data[i];
        for (size_t d = 0; d < D; d++) {
            columns[d].push_back(p[d]);
        }

        for (size_t c = 0; c < cubes_.size(); c++) {
            Datacube<D> dc = cubes_[c];
            columns[dc.index].push_back(dc.agg->operator()(p));
        }
    }
    num_columns_ = D + cubes.size();
    blocks_per_column_ = (size_ + COLUMN_COMPRESSION_BLOCK_SIZE_MASK) >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;

    cblocks_ = (CompressionBlock *)malloc(sizeof(CompressionBlock) * num_columns_ * blocks_per_column_);
    column_data_ = (char **)malloc(sizeof(char *) * num_columns_);
    compressed_column_sizes_.resize(num_columns_);

    Compress(columns);
    std::cout << "Dataset size: " << SizeInBytes() << " bytes" << std::endl;
}

// A hacky way to make a compressed dataset without cubes or primary key
template <size_t D>
CompressedColumnOrderDataset<D>::CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
                                                              bool data_only) {
    assert(data_only);
    size_ = data.size();
    std::vector<std::vector<Scalar>> columns(D);
    for (size_t i = 0; i < data.size(); i++) {
        const Point<D> &p = data[i];
        for (size_t d = 0; d < D; d++) {
            columns[d].push_back(p[d]);
        }
    }
    num_columns_ = D;
    blocks_per_column_ = (size_ + COLUMN_COMPRESSION_BLOCK_SIZE_MASK) >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;

    cblocks_ = (CompressionBlock *)malloc(sizeof(CompressionBlock) * num_columns_ * blocks_per_column_);
    column_data_ = (char **)malloc(sizeof(char *) * num_columns_);
    compressed_column_sizes_.resize(num_columns_);

    Compress(columns);
    std::cout << "Dataset size: " << SizeInBytes() << " bytes" << std::endl;
}

template <size_t D>
CompressedColumnOrderDataset<D>::CompressedColumnOrderDataset(const std::vector<Point<D>>& data,
        const std::vector<Datacube<D>>& cubes)
    : CompressedColumnOrderDataset<D>(data, std::vector<Scalar>(), cubes) {}

template <size_t D>
CompressedColumnOrderDataset<D>::~CompressedColumnOrderDataset() {
    free(cblocks_);
    for (size_t i = 0; i < num_columns_; i++) {
        free(column_data_[i]);
    }
    free(column_data_);
//    delete dsm_;
}

template <size_t D>
void CompressedColumnOrderDataset<D>::Compress(const std::vector<std::vector<Scalar>>& columns) {
    for (size_t i = 0; i < columns.size(); i++) {
        CompressColumn(i, columns[i]);
    }
}

template <size_t D>
void CompressedColumnOrderDataset<D>::CompressColumn(size_t dim_idx,
        const std::vector<Scalar>& column) {
    size_t delta = 1<<COLUMN_COMPRESSION_BLOCK_SIZE_POW;
    uint64_t offset = 0;
    std::vector<char> data_bits;
    std::vector<CompressionBlock> cblocks;
    for (size_t i = 0; i < column.size(); i += delta) {
        size_t end = std::min(column.size(), i + delta);
        const std::vector<Scalar> block(column.begin() + i, column.begin() + end);
        offset = CompressBlock(block, &cblocks, &data_bits, offset);
    }
    data_bits.shrink_to_fit();
    cblocks.shrink_to_fit();

    compressed_column_sizes_[dim_idx] = data_bits.size();
    std::cout << "Compressed column " << dim_idx << " to " << data_bits.size() << " bytes"
        << " with " << cblocks.size() << " blocks " << std::endl;
   
    assert (cblocks.size() == blocks_per_column_);
    // Store pointers to the underlying arrays (avoids extra vector indirection)
    std::memcpy(cblocks_ + dim_idx * blocks_per_column_, cblocks.data(), cblocks.size() * sizeof(CompressionBlock));
    column_data_[dim_idx] = (char *)malloc(data_bits.size());
    std::memcpy(column_data_[dim_idx], data_bits.data(), data_bits.size());
}

template <size_t D>
uint64_t CompressedColumnOrderDataset<D>::CompressBlock(const std::vector<Scalar>& block,
        std::vector<CompressionBlock>* cblocks,
        std::vector<char>* data_bits,
        uint64_t offset) {
    Scalar min_val = std::numeric_limits<Scalar>::max();
    Scalar max_val = std::numeric_limits<Scalar>::lowest();
    for (Scalar s : block) {
        min_val = std::min(s, min_val);
        max_val = std::max(s, max_val);
    }
    // base value is min, bit width is the number of bits needed to represent the difference max -
    // min
    Scalar diff = max_val - min_val;
    // TODO(vikram): replace with __builtin_clz or some other single instruction operation.
    char log2 = 0;
    while (diff > 0) {
        diff >>= 1;
        log2++;
    }
    CompressionBlock cblk;
    cblk.base_value = min_val;
    cblk.bit_offset = offset;
    cblk.bit_width = log2;
    cblk.bit_mask = ((1UL << log2) - 1UL);
    cblocks->push_back(cblk);
    //std::cout << "Compression block: base val = " << cblk.base_value << ", offset = " << cblk.bit_offset << ", bit_width = " << (int)cblk.bit_width << std::endl;

    // We need to insert starting at position `offset`.
    // Always make sure there's at least 64 bits of space when we insert the last element.
    uint64_t last_insertion = offset + block.size() * log2;
    size_t new_size_bytes = (last_insertion >> 3) + 1 + 8;
    // assert (new_size_bytes >= data_bits.size());
    data_bits->resize(new_size_bytes);

    for (size_t i = 0; i < block.size(); i++) {
        Scalar s = block[i];
        // Diff is guaranteed to be at most log2 bits.
        Scalar diff = s - min_val;
        size_t byte_ix = offset >> 3;
        uint64_t *bits = (uint64_t *)(data_bits->data() + byte_ix);
        char bit_offset = offset - (byte_ix << 3);
        
        uint64_t holder;
#ifdef LITTLE_ENDIAN_ORDER
        holder = __builtin_bswap64(*bits);
#endif
        holder |= (diff << ((sizeof(uint64_t) << 3) - log2 - bit_offset));
#ifdef LITTLE_ENDIAN_ORDER
        *bits = __builtin_bswap64(holder);
#endif
        offset += log2;
    }
    return offset;
}

template <size_t D>
Scalar CompressedColumnOrderDataset<D>::GetCoord(size_t index, size_t dim) const {
    const CompressionBlock cblk = cblocks_[dim * blocks_per_column_ + (index >> COLUMN_COMPRESSION_BLOCK_SIZE_POW)];
    size_t offset_in_block = index & COLUMN_COMPRESSION_BLOCK_SIZE_MASK;
    uint64_t bit_offset_from_start = cblk.bit_offset + cblk.bit_width * offset_in_block;
    // Get last 3 bits, which tell us which bit we need to start at.
    size_t bit_offset_from_byte_boundary = bit_offset_from_start & 0b111;
    // Extract the bits we need from it.
    size_t shift = 64 - cblk.bit_width - bit_offset_from_byte_boundary;
    uint64_t byte_block = *(uint64_t *)(column_data_[dim] + (bit_offset_from_start >> 3));
#ifdef LITTLE_ENDIAN_ORDER
    byte_block = __builtin_bswap64(byte_block);
#endif
    /*std::cout << "block index " << (index >> COLUMN_COMPRESSION_BLOCK_SIZE_POW)
        << ", cblk.bit_offset " << cblk.bit_offset
        << ", cblk.bit_width " << (int)cblk.bit_width
        << ", offset in block " << offset_in_block
        << ", bit_offset_from_start " << bit_offset_from_start
        << ", bit_offset_from_byte_boundary " << bit_offset_from_byte_boundary
        << ", shift " << shift
        << ", byteblock " << std::bitset<64>(byte_block)
        << ", mask " << std::bitset<64>(mask) << std::endl;*/
    return cblk.base_value + ((byte_block >> shift) & cblk.bit_mask);
} 

template <size_t D>
uint64_t CompressedColumnOrderDataset<D>::GetCoordRange(size_t start_ix, size_t end_ix, size_t dim, Scalar lower, Scalar upper) const {
    size_t cur_blk_ix = start_ix >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;
    const CompressionBlock cblk = cblocks_[dim * blocks_per_column_ + cur_blk_ix];
    size_t offset_in_block = start_ix & COLUMN_COMPRESSION_BLOCK_SIZE_MASK;
    // Initialize the offset to be for the first element at or after the current index that is
    // present. If we shift by 64, the result is undefined (according to the C++ standard), so we
    // need a special check if offset_in_block = 0.
    //offset_in_block = offset_in_block == 0 ? 0 : __builtin_popcountll(cblk.present >> (1 + COLUMN_COMPRESSION_BLOCK_SIZE_MASK - offset_in_block));
    uint64_t bit_offset_from_start = cblk.bit_offset + cblk.bit_width * offset_in_block;
    size_t end = std::min(end_ix, (cur_blk_ix+1) << COLUMN_COMPRESSION_BLOCK_SIZE_POW);
    uint64_t valids = 0; 
    for (size_t i = start_ix; i < end; i++) {  
        // Get last 3 bits, which tell us which bit we need to start at.
        size_t bit_offset_from_byte_boundary = bit_offset_from_start & 0b111;
        // Extract the bits we need from it.
        size_t shift = 64 - cblk.bit_width - bit_offset_from_byte_boundary;
        uint64_t byte_block = *(uint64_t *)(column_data_[dim] + (bit_offset_from_start >> 3));
#ifdef LITTLE_ENDIAN_ORDER
        byte_block = __builtin_bswap64(byte_block);
#endif
        Scalar val = cblk.base_value + ((byte_block >> shift) & cblk.bit_mask);
        valids <<= 1;
        valids |= val >= lower && val <= upper;
        bit_offset_from_start += cblk.bit_width;
    }
    if (end < end_ix) {
        // The range spans at most 2 blocks. Fetch the other block if necessary.
        uint64_t last_part = GetCoordRange(end, end_ix, dim, lower, upper);
        valids = (valids << (end_ix - end)) | last_part;
    }
    return valids;
}
    
template <size_t D>
void CompressedColumnOrderDataset<D>::GetRangeValues(size_t start_ix, size_t end_ix, size_t dim, uint64_t valids, std::vector<Scalar> *results) const {
    size_t cur_blk_ix = start_ix >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;
    const CompressionBlock cblk = cblocks_[dim * blocks_per_column_  + cur_blk_ix];
    size_t offset_in_block = start_ix & COLUMN_COMPRESSION_BLOCK_SIZE_MASK;
    uint64_t bit_offset_from_start = cblk.bit_offset + cblk.bit_width * offset_in_block;
    size_t end = std::min(end_ix, (cur_blk_ix+1) << COLUMN_COMPRESSION_BLOCK_SIZE_POW);
    uint64_t mask = 1UL << (end_ix - start_ix - 1);
    for (size_t i = start_ix; i < end; i++) {  
        // Get last 3 bits, which tell us which bit we need to start at.
        size_t bit_offset_from_byte_boundary = bit_offset_from_start & 0b111;
        // Extract the bits we need from it.
        size_t shift = 64 - cblk.bit_width - bit_offset_from_byte_boundary;
        uint64_t byte_block = *(uint64_t *)(column_data_[dim] + (bit_offset_from_start >> 3));
#ifdef LITTLE_ENDIAN_ORDER
        byte_block = __builtin_bswap64(byte_block);
#endif
        Scalar val = cblk.base_value + ((byte_block >> shift) & cblk.bit_mask);
        if (valids & mask) {
            results->push_back(val);    
        }
        mask >>= 1;
        bit_offset_from_start += cblk.bit_width;
    }
    if (end < end_ix) {
        // The range spans at most 2 blocks. Fetch the other block if necessary.
        GetRangeValues(end, end_ix, dim, valids, results);
    }
}
    
template <size_t D>
Scalar CompressedColumnOrderDataset<D>::GetRangeSum(size_t start_ix, size_t end_ix, size_t dim, uint64_t valids) const {
    size_t cur_blk_ix = start_ix >> COLUMN_COMPRESSION_BLOCK_SIZE_POW;
    const CompressionBlock cblk = cblocks_[dim * blocks_per_column_ + cur_blk_ix];
    size_t offset_in_block = start_ix & COLUMN_COMPRESSION_BLOCK_SIZE_MASK;
    uint64_t bit_offset_from_start = cblk.bit_offset + cblk.bit_width * offset_in_block;
    size_t end = std::min(end_ix, (cur_blk_ix+1) << COLUMN_COMPRESSION_BLOCK_SIZE_POW);
    uint64_t mask = 1UL << (end_ix - start_ix - 1);
    Scalar sum = 0;
    for (size_t i = start_ix; i < end; i++) {  
        if (valids & mask) {
            // Get last 3 bits, which tell us which bit we need to start at.
            size_t bit_offset_from_byte_boundary = bit_offset_from_start & 0b111;
            // Extract the bits we need from it.
            size_t shift = 64 - cblk.bit_width - bit_offset_from_byte_boundary;
            uint64_t byte_block = *(uint64_t *)(column_data_[dim] + (bit_offset_from_start >> 3));
#ifdef LITTLE_ENDIAN_ORDER
            byte_block = __builtin_bswap64(byte_block);
#endif
            Scalar val = cblk.base_value + ((byte_block >> shift) & cblk.bit_mask);
            sum += val;
        }
        mask >>= 1;
        bit_offset_from_start += cblk.bit_width;
    }
    if (end < end_ix) {
        // The range spans at most 2 blocks. Fetch the other block if necessary.
        sum += GetRangeSum(end, end_ix, dim, valids);
    }
    return sum;
}

template <size_t D>
Point<D> CompressedColumnOrderDataset<D>::Get(size_t ix) const {
    std::array<Scalar, D> pt;
    for (size_t i = 0; i < D; i++) {
        pt[i] = GetCoord(ix, i);
    }
    return pt;
}

template <size_t D>
Range<PhysicalIndex> CompressedColumnOrderDataset<D>::LookupRange(Range<Key> range) {
    auto startit = clustered_index_.lower_bound(range.start);
    auto endit = clustered_index_.lower_bound(range.end);
    return Range<PhysicalIndex>(startit->second, endit->second);
}

template <size_t D>
Set<PhysicalIndex> CompressedColumnOrderDataset<D>::Lookup(Set<Key> keys) {
    Set<PhysicalIndex> results;
    results.ranges.reserve(keys.ranges.size());
    results.list.reserve(keys.list.size());
    for (const auto& r : keys.ranges) {
        results.ranges.push_back(LookupRange(r));
    }
    for (const auto& r : keys.list) {
        auto loc = clustered_index_.find(r);
        AssertWithMessage(loc != clustered_index_.end(), "Search for invalid key");
        results.list.push_back(loc->second);
    }
    return results;
}


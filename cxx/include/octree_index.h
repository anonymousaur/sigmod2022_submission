#pragma once

#include <vector>
#include <memory>

#include "types.h"
#include "utils.h"
#include "primary_indexer.h"

/**
 * Splits space into eighths until each leaf node contains less than the page size number of points
 */
template <size_t D>
class OctreeIndex : public PrimaryIndexer<D> {
    public:
        //    Children follow a predictable pattern to make accesses simple.
        //    Here, - means less than 'origin' in that dimension, + means greater than.
        //    child:	0 1 2 3 4 5 6 7
        //    x:        - - - - + + + +
        //    y:        - - + + - - + +
        //    z:        - + - + - + - +
        struct Node : public PrimaryIndexNode {
            int32_t id;
            std::vector<std::shared_ptr<Node>> children;
            std::vector<Scalar> mins;
            std::vector<Scalar> maxs;  // inclusive
            PhysicalIndex start_offset;
            PhysicalIndex end_offset;

            Node() : children(), mins(), maxs() {}
            int32_t Id() override { return id; }
            PhysicalIndex StartOffset() override { return start_offset; }
            PhysicalIndex EndOffset() override { return end_offset; }
            std::vector<std::shared_ptr<PrimaryIndexNode>> Descendants() override {
                std::vector<std::shared_ptr<PrimaryIndexNode>> c(children.begin(), children.end());
                return c;
            }
        };

        explicit OctreeIndex(std::vector<size_t>& index_dims);
        OctreeIndex(std::vector<size_t>& index_dims, size_t page_size);

        virtual bool Init(PointIterator<D> start, PointIterator<D> end) override;
        Set<PhysicalIndex> IndexRanges(Query<D>&) override;
        size_t Size() const override;

        std::unordered_set<size_t> GetColumns() const override {
            return std::unordered_set<size_t>(index_dims_.cbegin(), index_dims_.cend());
        }

        void WriteStats(std::ofstream& statsfile) override {
            statsfile << "primary_index_type: octree_";
            for (size_t d : index_dims_) {
                statsfile << d << "_";
            }
            statsfile << "p" << page_size_ << std::endl;
        } 

    private:
        std::vector<size_t> index_dims_;
        size_t page_size_;
        std::shared_ptr<Node> root_node;
        bool sort_leaf_;
        size_t sort_dim_;
        size_t data_size_;
        int32_t next_id_;

        std::vector<Scalar> mins_;
        std::vector<Scalar> maxs_;

        int get_octant_containing_point(Point<D>& point, std::vector<Scalar>& center) const;
        bool divide_node(std::shared_ptr<Node> node, PointIterator<D> start, PointIterator<D> end, int depth);
        bool should_keep_dividing(std::shared_ptr<Node> node, int depth) const;
        bool is_relevant_node(std::shared_ptr<Node> node, const Query<D>& query) const;
        size_t num_partitions_;

        static const size_t DEFAULT_PAGE_SIZE = 10000;
        static const int DEFAULT_MAX_DEPTH = 100;
        std::ofstream sorted_data_points_;
        std::ofstream sorted_data_buckets_; 
                
};

#include "../src/octree_index.hpp"

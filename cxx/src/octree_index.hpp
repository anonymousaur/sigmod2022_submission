#include <cmath>
#include <cassert>
#include <bitset>
#include <set>
#include <stack>
#include "octree_index.h"

using namespace std;

template <size_t D>
OctreeIndex<D>::OctreeIndex(std::vector<size_t>& index_dims) :
    OctreeIndex<D>(index_dims, DEFAULT_PAGE_SIZE) {}

template <size_t D>
OctreeIndex<D>::OctreeIndex(std::vector<size_t>& index_dims, size_t page_size) :
        index_dims_(index_dims),
        page_size_(page_size),
        sort_leaf_(false),
        mins_(index_dims.size()),
        maxs_(index_dims.size()),
        next_id_(0),
        sorted_data_points_(),
        sorted_data_buckets_(),
        num_partitions_(1U << index_dims.size()) {
            sorted_data_points_.open("octree_sorted_data.bin", std::ios::binary);
            sorted_data_buckets_.open("octree_sorted_buckets.dat");
        }


template <size_t D>
int OctreeIndex<D>::get_octant_containing_point(Point<D>& point, std::vector<Scalar>& center) const {
    int oct = 0;
    for (size_t i = 0; i < index_dims_.size(); i++) {
        if(point[index_dims_[i]] > center[i]) {
            oct |= 1 << (index_dims_.size() - i - 1);
        }
    }
//    if(point[index_dims_[0]] > center[0]) oct |= 4;
//    if(point[index_dims_[1]] > center[1]) oct |= 2;
//    if(point[index_dims_[2]] > center[2]) oct |= 1;
    return oct;
}

template <size_t D>
bool OctreeIndex<D>::should_keep_dividing(std::shared_ptr<Node> node, int depth) const {
    if (depth == DEFAULT_MAX_DEPTH) {
        return false;
    }
    if (node->end_offset - node->start_offset <= page_size_) {
        return false;
    }
    bool single_point = true;
    for (size_t i = 0; i < index_dims_.size(); i++) {
        if (node->mins[i] < node->maxs[i]) {
            single_point = false;
            break;
        }
    }
    return !single_point;
}

template <size_t D>
bool OctreeIndex<D>::divide_node(std::shared_ptr<Node> node, PointIterator<D> data_start, PointIterator<D> data_end, int depth) {
    if (!should_keep_dividing(node, depth)) {
        if (sort_leaf_) {
            std::sort(data_start + node->start_offset, data_start + node->end_offset,
                      [&](const Point<D>& a, const Point<D>& b) -> bool {
                          return a[sort_dim_] < b[sort_dim_];
                      });
        }
        sorted_data_buckets_ << node->start_offset << ", " << node->end_offset << std::endl;
        size_t write_size = sizeof(Point<D>) * (node->end_offset - node->start_offset);
        sorted_data_points_.write((char *)&*(data_start + node->start_offset), write_size);
        sorted_data_points_.flush();
        return sort_leaf_;
    }

    std::vector<Scalar> center(index_dims_.size());
    for (size_t i = 0; i < index_dims_.size(); i++) {
        center[i] = (node->mins[i] + node->maxs[i]) / 2;
    }
    /*if (depth < 2) {
        std::cout << "Node at depth " << depth << ", center: {";
        for (Scalar c : center) {
            std::cout << " " << c;
        }
        std::cout << "}" << std::endl;
    }*/

//    std::cout << "divide depth " << depth << " npoints " << node->end_offset - node->start_offset << " center ";
//    for (size_t i = 0; i < index_dims_.size(); i++) {
//        std::cout << center[i] << " ";
//    }
//    std::cout << std::endl;

    std::vector<std::vector<Point<D>>> child_data(num_partitions_);
    node->children = std::vector<std::shared_ptr<Node>>(num_partitions_);
    for (size_t i = 0; i < num_partitions_; i++) {
        std::vector<Scalar> child_mins(index_dims_.size());
        std::vector<Scalar> child_maxs(index_dims_.size());
        for (size_t d = 0; d < index_dims_.size(); d++) {
            if (i & (1 << (index_dims_.size() - d - 1))) {
                child_mins[d] = center[d] + 1;
                child_maxs[d] = node->maxs[d];
            } else {
                child_mins[d] = node->mins[d];
                child_maxs[d] = center[d];
            }
        }
        node->children[i] = std::make_shared<Node>();
        node->children[i]->id = next_id_++;
        node->children[i]->mins = child_mins;
        node->children[i]->maxs = child_maxs;
    }
    /*if (depth == 0) {
        for (size_t i = 0; i < num_partitions_; i++) {
            std::cout << "Child " << i << ": mins = {";
            for (Scalar m : node->children[i]->mins) {
                std::cout << " " << m;
            }
            std::cout << "}, maxs = {";
            for (Scalar m : node->children[i]->maxs) {
                std::cout << " " << m;
            }
            std::cout << "}";
        }
    }*/
    
    // Data is unmodified at this level if the points are already sorted by octant.
    bool data_modified = false;
    size_t prev_oct = 0;
    for (size_t i = node->start_offset; i < node->end_offset; i++) {
        auto pt = *(data_start + i);
        size_t oct = get_octant_containing_point(pt, center);
        child_data[oct].push_back(pt);
        /*if (oct < prev_oct) {
            std::cout << "Got bucket " << oct << " after " << prev_oct << ", depth " << depth << std::endl;
        }*/
        data_modified |= (oct < prev_oct);
        prev_oct = oct;
    }

    size_t cur = node->start_offset;
    for (size_t i = 0; i < num_partitions_; i++) {
        node->children[i]->start_offset = cur;
        node->children[i]->end_offset = cur + child_data[i].size();
        if (data_modified) {
            for (size_t j = 0; j < child_data[i].size(); j++) {
                *(data_start + cur) = child_data[i][j];
                cur++;
            }
        } else {
            cur += child_data[i].size();
        }
        // Free the unnecessary memory
        std::vector<Point<D>>().swap(child_data[i]);
    }

    for (size_t i = 0; i < num_partitions_; i++) {
        if (node->children[i]->start_offset == node->children[i]->end_offset) {
            node->children[i] = nullptr;
        } else {
            bool mod = divide_node(node->children[i], data_start, data_end, depth + 1);
            data_modified |= mod;
        }
    }
    return data_modified;
}

// Modifies data in place to sort it via this indexing method.
template <size_t D>
bool OctreeIndex<D>::Init(PointIterator<D> data_start, PointIterator<D> data_end) {
    for (size_t i = 0; i < index_dims_.size(); i++) {
        mins_[i] = std::numeric_limits<Scalar>::max();
        maxs_[i] = std::numeric_limits<Scalar>::lowest();
    }
    for (auto it = data_start; it != data_end; it++) {
        const Point<D> p = *it;
        for (size_t i = 0; i < index_dims_.size(); i++) {
            mins_[i] = std::fmin(p[index_dims_[i]], mins_[i]);
            maxs_[i] = std::fmax(p[index_dims_[i]], maxs_[i]);
        }
    }
    std::cout << "Building Octree with page size " << page_size_ 
        << " on dimensions: ";
    for (size_t i : index_dims_) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    data_size_ = std::distance(data_start, data_end);
    root_node = std::make_shared<Node>();
    root_node->id = next_id_++;
    root_node->mins = mins_;
    root_node->maxs = maxs_;
    root_node->start_offset = 0;
    root_node->end_offset = std::distance(data_start, data_end);
    bool modified = divide_node(root_node, data_start, data_end, 0);
    std::cout << "Data was " << (modified ? "" : "not ") << "modified" << std::endl;
    sorted_data_points_.close();
    sorted_data_buckets_.close();
    return modified;
}

template <size_t D>
bool OctreeIndex<D>::is_relevant_node(std::shared_ptr<Node> node, const Query<D>& query) const {
    for (size_t i = 0; i < index_dims_.size(); i++) {
        QueryFilter qf = query.filters[index_dims_[i]];
        if (!qf.present) {
            continue;
        }
        assert (qf.is_range);
        // This means that the range on this dimension is empty.
        if (qf.ranges.size() == 0 || qf.ranges[0].first > qf.ranges[0].second) {
            return false;
        }
        if (qf.ranges[0].first > node->maxs[i] || qf.ranges[0].second < node->mins[i]) {
            return false;
        }
    }
    return true;
}


template <size_t D>
Set<PhysicalIndex> OctreeIndex<D>::IndexRanges(Query<D> &query) {
    Ranges<PhysicalIndex> ranges;
    bool index_relevant = false;
    for (size_t dim : index_dims_) {
        index_relevant |= query.filters[dim].present;
    }
    if (!index_relevant) {
        std::cout << "Index is not relevant...returning full scan" << std::endl;
        return {{{0, data_size_}}, {}};
    }
    std::cout << "Index is relevant" << std::endl;
    std::stack<std::shared_ptr<Node>> node_stack;
    node_stack.push(root_node);
    size_t indexes_scanned = 0;
    while (!node_stack.empty()) {
        std::shared_ptr<Node> cur = node_stack.top();
        node_stack.pop();
        if (!is_relevant_node(cur, query)) {
            continue;
        }
        if (cur->children.empty()) {
            // this node is a leaf
            // TODO: add code here if we're sorting.
            if (cur->end_offset > cur->start_offset) {
                ranges.emplace_back(cur->start_offset, cur->end_offset);
            }
            indexes_scanned += cur->end_offset - cur->start_offset;
        } else {
            for (auto it = cur->children.rbegin(); it != cur->children.rend(); it++) {
                if (*it != nullptr) {
                    node_stack.push(*it);
                }
            }
        }
    }
    return {ranges, {}};
}

template <size_t D>
size_t OctreeIndex<D>::Size() const {
    uint64_t size = 0;
    size += mins_.size() * sizeof(Scalar) + maxs_.size() * sizeof(Scalar);

    size_t num_leaf_nodes = 0;
    size_t num_inner_nodes = 0;
    size_t max_page_size = 0;
    size_t leaf_node_size = sizeof(Node) + sizeof(Scalar) * 2 * index_dims_.size();
    size_t inner_node_size = leaf_node_size + sizeof(std::shared_ptr<Node*>) * num_partitions_;
    std::stack<Node*> node_stack;
    node_stack.push(root_node.get());
    while (!node_stack.empty()) {
        Node* cur = node_stack.top();
        node_stack.pop();
        if (cur->children.empty()) {
            size += leaf_node_size;
            num_leaf_nodes++;
            max_page_size = std::max(max_page_size, cur->end_offset - cur->start_offset);
        } else {
            size += inner_node_size;
            for (auto it = cur->children.rbegin(); it != cur->children.rend(); it++) {
                if (*it) {
                    node_stack.push((*it).get());
                }
            }
            num_inner_nodes++;
        }
    }

    std::cout << "Leaf nodes: " << num_leaf_nodes << ", inner nodes: " << num_inner_nodes << ", max page size: " <<
              max_page_size << std::endl;

    return size;
}


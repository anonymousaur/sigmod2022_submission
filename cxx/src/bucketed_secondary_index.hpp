#include "bucketed_secondary_index.h"

#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>

#include "merge_utils.h"
#include "file_utils.h"
#include "utils.h"

template <size_t D>
BucketedSecondaryIndex<D>::BucketedSecondaryIndex(size_t dim)
    : BucketedSecondaryIndex<D>(dim, false, {}) {}

template <size_t D>
BucketedSecondaryIndex<D>::BucketedSecondaryIndex(size_t dim, List<Key> subset)
    : BucketedSecondaryIndex<D>(dim, true, subset) {}

template <size_t D>
BucketedSecondaryIndex<D>::BucketedSecondaryIndex(size_t dim, bool use_subset, List<Key> subset)
    : SecondaryIndexer<D>(dim),
      bucket_width_(0),
      bucket_strat_(UNSET),
      index_subset_(subset),
      use_subset_(use_subset),
      buckets_() {}

template <size_t D>
void BucketedSecondaryIndex<D>::Init(ConstPointIterator<D> start, ConstPointIterator<D> end) {
    assert (bucket_strat_ != UNSET);
    data_size_ = std::distance(start, end);
    std::cout << "Begin SecondaryIndex init" << std::endl;
    if (!use_subset_) {
        Key i = 0;
        indexed_size_ = data_size_;
        for (auto it = start; it != end; it++, i++) {
            InsertUnsorted(i, (*it)[this->column_]);
        }
    } else {
        indexed_size_ = index_subset_.size();
        for (Key ix : index_subset_) {
            InsertUnsorted(ix, (*(start+ix))[this->column_]);
        }
        index_subset_.clear();
    }
    std::cout << "Finished building map" << std::endl;
    size_t minsize = std::numeric_limits<size_t>::max();
    size_t maxsize = 0;
    for (auto it = buckets_.begin(); it != buckets_.end(); it++) {
        std::sort(it->second.begin(), it->second.end());
        minsize = std::min(minsize, it->second.size());
        maxsize = std::max(maxsize, it->second.size());
    }
    std::cout << "Finished creating BucketedSecondaryIndex with " << buckets_.size()
        << " buckets, smallest = " << minsize << ", largest = " << maxsize << std::endl;
}

template <size_t D>
void BucketedSecondaryIndex<D>::SetBucketFile(const std::string& filename) {
    std::ifstream file(filename);
    AssertWithMessage(file.is_open(), "Couldn't find file " + filename);
    AssertWithMessage(FileUtils::NextLine(file) == "continuous-0", "Bad input " + filename);
    
    auto header = FileUtils::NextArray<std::string>(file, 3);
    AssertWithMessage(header[0] == "source", "Bad input " + filename);
    //AssertWithMessage(this->column_ == std::stoi(header[1]),
    //        "Mapping file dimension does not match spec");
    size_t s = std::stoi(header[2]);
    // The last element is the rightmost bound on the data.
    // Read everything as a double to be safe and then convert to scalar
    
    std::cout << "BucketedSecondaryIndex: reading " << s << " mapped buckets" << std::endl;
    for (size_t i = 0; i < s; i++) {
        auto arr = FileUtils::NextArray<double>(file, 3);
        Scalar bucket_start = (Scalar)arr[1];
        buckets_.insert(std::make_pair(bucket_start, List<Key>{}));
    }
    file.close();
    bucket_strat_ = FROM_FILE;
}

template <size_t D>
void BucketedSecondaryIndex<D>::SetBucketWidth(Scalar width) {
    bucket_width_ = width;
    bucket_strat_ = CONST_WIDTH;
}

template <size_t D>
void BucketedSecondaryIndex<D>::InsertUnsorted(Key index, Scalar v)  {
    auto loc = buckets_.end();
    if (bucket_strat_ == CONST_WIDTH) {
        Scalar bucket = (Scalar)(floor(v / bucket_width_));
        Scalar key = bucket * bucket_width_;
        // Returns an iterator to the existing element if an element already exists.
        loc = buckets_.insert(std::make_pair(key, List<Key>{})).first;
    } else if (bucket_strat_ == FROM_FILE) {
        loc = buckets_.upper_bound(v);
        AssertWithMessage(loc != buckets_.begin(), "Invalid bucket list: got point before first bucket");
        loc--;
    }
    loc->second.push_back(index);
     
}

template <size_t D>
List<Key> BucketedSecondaryIndex<D>::Matches(const Query<D>& q) const {
    auto filter = q.filters[this->column_];
    assert (filter.present && filter.is_range);
    // Store a list of the index lists to merge all at once.
    std::vector<const List<Key> *> idx_lists;
    size_t max_size = 0;
    size_t smallest_vec = std::numeric_limits<size_t>::max();
    size_t largest_vec = 0;
    for (ScalarRange r : filter.ranges) {
        auto startit = buckets_.upper_bound(r.first);
        if (startit != buckets_.begin()) {
            startit--;
        }
        auto endit = buckets_.upper_bound(r.second);
        for (auto it = startit; it != endit; it++) {
            if (it->second.size() == 0) {
                continue;
            }
            idx_lists.push_back(&(it->second));
            size_t s = it->second.size();
            smallest_vec = std::min(s, smallest_vec);
            largest_vec = std::max(s, largest_vec);
            max_size += s;
        }
    }
     std::cout << "Unioning " << idx_lists.size() << " index lists"
         << " with " << max_size << " indexes. Smallest = " 
         << smallest_vec << ", largest = " << largest_vec << std::endl;
    // std::cout << "Bucket sizes: ";
    // for (auto v : idx_lists) {
    //     std::cout << v->size() << " ";
    // }
    // std::cout << std::endl;

    // Different merge solutions based on size. After a small number of lists, it's more efficient
    // to just resort everything, since std::sort is so fast.
    if (idx_lists.size() == 1) {
        return *(idx_lists[0]);
    }
    if (idx_lists.size() == 2) {
        List<Key> all_ixs;
        all_ixs.reserve(max_size);
        std::merge(idx_lists[0]->begin(), idx_lists[0]->end(),
                idx_lists[1]->begin(), idx_lists[1]->end(),
                std::back_inserter(all_ixs));
        return all_ixs;
    } 
    if (idx_lists.size() <= 5) {
        return MergeUtils::Union(idx_lists);
    } else {
        List<Key> all_ixs;
        all_ixs.reserve(max_size);
        for (const List<Key>* ixl : idx_lists) {
            all_ixs.insert(all_ixs.end(), ixl->begin(), ixl->end());
        }
        std::sort(all_ixs.begin(), all_ixs.end());
        return all_ixs;
    }
}

template <size_t D>
void BucketedSecondaryIndex<D>::Insert(const std::vector<KeyPair>& inserts) {
    if (inserts.empty()) {
        return;
    }
    // Assumes inserts are sorted by .first
    auto mapit = buckets_.upper_bound(inserts[0].first);
    auto rb = buckets_.upper_bound(inserts.back().first);
    auto mapp1it = mapit;
    mapit--;
    for (auto it = inserts.begin(); it != inserts.end(); it++) {
        while (mapp1it != rb && mapp1it->first < it->first) {
            mapit++;
            mapp1it++;
        } 
        mapit->second.push_back(it->second);
    }
}

template <size_t D>
void BucketedSecondaryIndex<D>::Remove(const ScalarRange& scalar_range, const Range<Key>& key_range) {
    auto lb = buckets_.lower_bound(scalar_range.first);
    auto rb = buckets_.lower_bound(scalar_range.second);
    for (auto it = lb; it != rb; it++) {
        std::vector<Key> leftover;
        leftover.reserve(it->second.size());
        for (auto vit = it->second.begin(); vit != it->second.end(); vit++) {
            if (*vit >= key_range.start && *vit < key_range.end) {
                leftover.push_back(*vit);
            }
        }
        it->second = std::move(leftover);
    }
}

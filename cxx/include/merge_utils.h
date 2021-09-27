
#pragma once

#include <vector>

#include "types.h"


class MergeUtils {
  private:
    MergeUtils() {}

  public:
    static std::vector<ScalarRange> Intersect(const std::vector<ScalarRange>&, const std::vector<ScalarRange>&);
   
    template <typename T> 
    static List<T> Intersect(const List<T>&, const List<T>&);
    
    template <typename T>
    static Set<T> Intersect(const Set<T>& set1, const Set<T>& set2);

    template <typename T>
    static Set<T> Intersect(const Ranges<T>&, const List<T>&);
    
    
    template <typename T>
    static Set<T> Union(const Ranges<T>&, const List<T>&);

    template <typename T>
    static Ranges<T> Intersect(const Ranges<T>&, const Ranges<T>&);
    
    //// Note: this does NOT deduplicate.
    template <typename T>
    static List<T> Union(const std::vector<const List<T> *> ix_lists);
    //// Scalar ranges must be sorted.
    template <class ForwardIterator>
    static std::vector<ScalarRange> Coalesce(ForwardIterator begin, ForwardIterator end);
};

#include "../src/merge_utils.hpp"

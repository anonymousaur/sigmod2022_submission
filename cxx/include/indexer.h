#pragma once

#include <iterator>
#include <fstream>
#include <algorithm>
#include <vector>

#include "types.h"
#include "dataset.h"
#include "utils.h"

template <size_t D>
class Indexer {
  public:

    Indexer<D>() {}
        
    // Size of the indexer in bytes
    virtual size_t Size() const = 0;

    virtual IndexerType Type() const = 0;

    // Dummy implementation since only a few indexers will actually let us insert.
    std::vector<PhysicalIndex> Insert(std::vector<Point<D>> points) {
        AssertWithMessage(false, "Indexer does not allow inserts");
        return {};
    }

    virtual void WriteStats(std::ofstream& statsfile) {}
    
};

#pragma once

#include <fstream>

#include "indexer.h"
#include "types.h"

template <size_t D>
class Rewriter : public Indexer<D> {
  public:

    // Rewrite the query for the primary index.
    // Returns any auxiliary indexes that have to be scanned as a result.
    virtual List<Key> Rewrite(Query<D>& q) = 0;

    virtual void Init(ConstPointIterator<D> start, ConstPointIterator<D> end) = 0;

    virtual size_t Size() const = 0;

    virtual IndexerType Type() const { return Rewriting; }

    virtual void WriteStats(std::ofstream& statsfile) override {}
};


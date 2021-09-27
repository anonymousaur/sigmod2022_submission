#include "gtest/gtest.h"
#include "composite_index.h"

#include "secondary_btree_index.h"
#include "primary_btree_index.h"
#include <vector>

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class CompositeIndexTest : public ::testing::Test {
        public:
        vector<Point<TESTD>> ValuesToPoints(const std::vector<Scalar>& vals) {
            vector<Point<TESTD>> pts;
            for (Scalar s : vals) {
                pts.push_back({s, 0});
            }
            return pts;
        }

        template <typename T>
        bool ArrayEqual(const std::vector<T>& got, const std::vector<T>& want) {
            bool match = (got.size() == want.size());
            if (match) {
                for (size_t i = 0; i < got.size(); i++) {
                    match &= (got[i] == want[i]);
                }
            }
            if (!match) {
                std::cout << "Got " << VectorToString(got) << " but wanted "
                   << VectorToString(want) << std::endl; 
            }
            return match;
        }
    };

    TEST_F(CompositeIndexTest, TestInitWithPrimary) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.Init(pts.begin(), pts.end());
        vector<Scalar> want = {1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pts[i][0], want[i]);
        }
    }
    
    TEST_F(CompositeIndexTest, TestInitWithSecondary) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(0);
        CompositeIndex<TESTD> index(1);
        index.AddSecondaryIndex(std::move(sindex));
        std::cout << "Check 1" << std::endl;
        index.Init(pts.begin(), pts.end());
        std::cout << "Check 2" << std::endl;
        // Ordering should remain unchanged.
        vector<Scalar> want = {10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pts[i][0], want[i]);
        }
    }

    TEST_F(CompositeIndexTest, TestRangesWithoutRelevantFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[0] = {.present = false};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {2}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        // Return the whole range because the filter doesn't cover the relevant dimension (0). 
        std::vector<Range<PhysicalIndex>> want = {{0, 12}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
    
    TEST_F(CompositeIndexTest, TestRangesWithSecondaryFilter) {
        std::vector<Point<TESTD>> pts = {{10, 10}, {6, 6}, {6, 6}, {7, 7}, {8, 8}, {2, 2}, {3, 3},
            {5, 5}, {1, 1}, {2, 2}, {9, 9}, {4, 4}};
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> index(1);
        index.AddSecondaryIndex(std::move(sindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[0] = {.present = false};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1, 2}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{5, 6}, {8, 10}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
    
    TEST_F(CompositeIndexTest, TestRangesWithPrimaryIndexButSecondaryFilter) {
        std::vector<Point<TESTD>> pts = {{10, 10}, {6, 6}, {6, 6}, {7, 7}, {8, 8}, {2, 2}, {3, 3},
            {5, 5}, {1, 1}, {2, 2}, {9, 9}, {4, 4}};
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.AddSecondaryIndex(std::move(sindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[0] = {.present = false};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1, 2}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{0, 3}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
    
    TEST_F(CompositeIndexTest, TestRangesRelevantPrimaryFilter) {
        std::vector<Point<TESTD>> pts = {{10, 10}, {6, 6}, {6, 6}, {7, 7}, {8, 8}, {2, 2}, {3, 3},
            {5, 5}, {1, 1}, {2, 2}, {9, 9}, {4, 4}};
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.AddSecondaryIndex(std::move(sindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {1, 2}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{0, 3}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
    
    TEST_F(CompositeIndexTest, TestRangesPrimaryAndSecondaryFilter) {
        std::vector<Point<TESTD>> pts = {{10, 10}, {6, 6}, {6, 6}, {7, 7}, {8, 8}, {2, 2}, {3, 3},
            {5, 5}, {1, 1}, {2, 2}, {9, 9}, {4, 4}};
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.AddSecondaryIndex(std::move(sindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2, 3, 4, 5}};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1, 2, 5}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{1, 3}, {5, 6}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
    
    TEST_F(CompositeIndexTest, TestRangesWithLargerGapThreshold) {
        std::vector<Point<TESTD>> pts = {{10, 10}, {6, 6}, {6, 6}, {7, 7}, {8, 8}, {2, 2}, {3, 3},
            {5, 5}, {1, 1}, {2, 2}, {9, 9}, {4, 4}};
        auto pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        auto sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> index(1);
        index.SetPrimaryIndex(std::move(pindex));
        index.AddSecondaryIndex(std::move(sindex));
        index.Init(pts.begin(), pts.end());
        
        Query<TESTD> q;
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2, 3, 4, 5}};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1, 2, 3, 5}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{1, 4}, {5, 6}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
   
        pindex = std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1);
        sindex = std::make_unique<SecondaryBTreeIndex<TESTD>>(1);
        CompositeIndex<TESTD> gapIndex(2);
        gapIndex.SetPrimaryIndex(std::move(pindex));
        gapIndex.AddSecondaryIndex(std::move(sindex));
        gapIndex.Init(pts.begin(), pts.end());
         
        ranges = gapIndex.IndexRanges(q).ranges;
        // Even though 4 isn't included in the final output.
        want = {{1, 6}};
        EXPECT_TRUE(ArrayEqual(ranges, want));
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


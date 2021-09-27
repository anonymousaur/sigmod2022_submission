#include "gtest/gtest.h"
#include "primary_btree_index.h"
#include <vector>

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class PrimaryBTreeIndexTest : public ::testing::Test {
        public:
        using Page = PrimaryBTreeIndex<TESTD>::Page;
        vector<Point<TESTD>> ValuesToPoints(const std::vector<Scalar>& vals) {
            vector<Point<TESTD>> pts;
            for (Scalar s : vals) {
                pts.push_back({s, 0});
            }
            return pts;
        }
    };

    TEST_F(PrimaryBTreeIndexTest, TestInit) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        vector<Scalar> want = {1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pts[i][0], want[i]);
        }
    }

    TEST_F(PrimaryBTreeIndexTest, TestRangesWithoutFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[0] = {.present = false};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{0, 12}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }

    TEST_F(PrimaryBTreeIndexTest, TestRangesWithFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,4}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{1, 3}, {4, 5}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }
    
    TEST_F(PrimaryBTreeIndexTest, TestAdjacentRanges) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,3}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        // Should merge [1, 3] and [3, 4]
        std::vector<Range<PhysicalIndex>> want = {{1, 4}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }

    TEST_F(PrimaryBTreeIndexTest, TestRangesValueNotPresent) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 3});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {4}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        EXPECT_EQ(ranges.size(), 0);
    }

    TEST_F(PrimaryBTreeIndexTest, TestPages) {
        auto pts = ValuesToPoints({2, 3, 2, 4, 6});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        // Order of values is {2, 2, 3, 4, 6}
        auto pages = index.Pages();
        std::vector<Page> want = {Page({2, 2}, {0, 2}), Page({3, 3}, {2, 3}),
                                   Page({4, 4}, {3, 4}), Page({6, 6}, {4, 5})};
        EXPECT_EQ(pages.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pages[i].value_range, want[i].value_range);
            EXPECT_EQ(pages[i].index_range, want[i].index_range);
        } 
    }
    
    TEST_F(PrimaryBTreeIndexTest, TestPagesEndWithDuplicate) {
        auto pts = ValuesToPoints({2, 3, 2, 4, 6, 6});
        PrimaryBTreeIndex<TESTD> index(0, 1);
        index.Init(pts.begin(), pts.end());
        // Order of values is {2, 2, 3, 4, 6}
        auto pages = index.Pages();
        std::vector<Page> want = {Page({2, 2}, {0, 2}), Page({3, 3}, {2, 3}),
                                   Page({4, 4}, {3, 4}), Page({6, 6}, {4, 6})};
        EXPECT_EQ(pages.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pages[i].value_range, want[i].value_range);
            EXPECT_EQ(pages[i].index_range, want[i].index_range);
        } 
    }
    
    TEST_F(PrimaryBTreeIndexTest, TestPagesLargerPageSize) {
        auto pts = ValuesToPoints({2, 3, 2, 4, 6, 7, 7, 8, 10});
        PrimaryBTreeIndex<TESTD> index(0, 2);
        index.Init(pts.begin(), pts.end());
        // Order of values is {2, 2, 3, 4, 6, 7, 7, 8, 10}
        auto pages = index.Pages();
        std::vector<Page> want = {Page({2, 2}, {0, 2}), Page({3, 4}, {2, 4}),
                                   Page({6, 6}, {4, 5}), Page({7, 7}, {5, 7}),
                                   Page({8, 10}, {7, 9})};
        EXPECT_EQ(pages.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pages[i].value_range, want[i].value_range);
            EXPECT_EQ(pages[i].index_range, want[i].index_range);
        } 
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


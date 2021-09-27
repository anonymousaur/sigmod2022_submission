#include "gtest/gtest.h"
#include "binary_search_index.h"
#include <vector>
#include <iostream>

#include "utils.h"

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class BinarySearchIndexTest : public ::testing::Test {
        public:
        vector<Point<TESTD>> ValuesToPoints(const std::vector<Scalar>& vals) {
            vector<Point<TESTD>> pts;
            for (Scalar s : vals) {
                pts.push_back({s, 0});
            }
            return pts;
        }

    };

    TEST_F(BinarySearchIndexTest, TestInit) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        BinarySearchIndex<TESTD> index(0);
        index.Init(pts.begin(), pts.end());
        vector<Scalar> got;
        for (const Point<TESTD>& p : pts) {
            got.push_back(p[0]);
        }
        vector<Scalar> want = {1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(got[i], want[i]) << "Got " << ArrayToString(got)
                << " but want " << ArrayToString(want);
        }
    }

    TEST_F(BinarySearchIndexTest, TestRangesWithoutFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        BinarySearchIndex<TESTD> index(0);
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

    TEST_F(BinarySearchIndexTest, TestRangesWithFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        BinarySearchIndex<TESTD> index(0);
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
    
    TEST_F(BinarySearchIndexTest, TestAdjacentRanges) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        BinarySearchIndex<TESTD> index(0);
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
    
    TEST_F(BinarySearchIndexTest, TestRangesValueNotPresent) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 3});
        BinarySearchIndex<TESTD> index(0);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {4}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        EXPECT_EQ(ranges.size(), 0);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


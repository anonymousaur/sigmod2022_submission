#include "gtest/gtest.h"
#include "secondary_btree_index.h"
#include <vector>

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class SecondaryBTreeIndexTest : public ::testing::Test {
        public:
        vector<Point<TESTD>> ValuesToPoints(const std::vector<Scalar>& vals) {
            vector<Point<TESTD>> pts;
            for (Scalar s : vals) {
                pts.push_back({s, 0});
            }
            return pts;
        }
    };

    TEST_F(SecondaryBTreeIndexTest, TestInit) {
        vector<Point<TESTD>> pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        auto want = pts;
        SecondaryBTreeIndex<TESTD> index(0);
        index.Init(pts.begin(), pts.end());
        for (size_t i = 0; i < want.size(); i++) {
            // No change to underlying data.
            EXPECT_EQ(pts[i][0], want[i][0]);
        }
    }

    TEST_F(SecondaryBTreeIndexTest, TestRangesWithoutFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        SecondaryBTreeIndex<TESTD> index(0);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[0] = {.present = false};
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {1}};
        std::vector<Key> ranges = index.Matches(q);
        std::vector<Key> want = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i], want[i]);
        }
    }

    TEST_F(SecondaryBTreeIndexTest, TestRangesWithFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        SecondaryBTreeIndex<TESTD> index(0);
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,4}};
        std::vector<Key> ranges = index.Matches(q);
        std::sort(ranges.begin(), ranges.end());
        std::vector<Key> want = {5, 9, 11};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i], want[i]);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


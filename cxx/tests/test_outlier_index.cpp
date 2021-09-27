#include "gtest/gtest.h"
#include "outlier_index.h"
#include "primary_btree_index.h"
#include <vector>

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class OutlierIndexTest : public ::testing::Test {
        public:
        vector<Point<TESTD>> ValuesToPoints(const std::vector<Scalar>& vals) {
            vector<Point<TESTD>> pts;
            for (Scalar s : vals) {
                pts.push_back({s, 0});
            }
            return pts;
        }
    };

    TEST_F(OutlierIndexTest, TestInit) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        
        index.Init(pts.begin(), pts.end());
        vector<Scalar> want = {1, 4, 6, 6, 8, 9, 10};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pts[i][0], want[i]);
        }
    }

    TEST_F(OutlierIndexTest, TestRangesWithoutFilter) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        
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

    TEST_F(OutlierIndexTest, TestRangesWithFilterInlierMatch) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,4}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{1, 2}, {7, 12}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }
    
    TEST_F(OutlierIndexTest, TestRangesWithFilterNoInlierMatch) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        
        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,3}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{7, 12}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }

    TEST_F(OutlierIndexTest, TestInitWithOutlierIndex) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        index.SetOutlierIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        
        index.Init(pts.begin(), pts.end());
        vector<Scalar> want = {1, 4, 6, 6, 8, 9, 10, 2, 2, 3, 5, 7};
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(pts[i][0], want[i]);
        }
    }
    
    TEST_F(OutlierIndexTest, TestRangesWithOutlierIndexAndInlierMatch) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        index.SetOutlierIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));

        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {2,4}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{1, 2}, {7, 9}};
        EXPECT_EQ(ranges.size(), want.size()) << "Got " << RangesToString(ranges)
            << " but want " << RangesToString(want);
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }

    TEST_F(OutlierIndexTest, TestRangesWithOutlierIndexNoInlierMatch) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 4});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        index.SetOutlierIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));

        index.Init(pts.begin(), pts.end());
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {3}};
        std::vector<Range<PhysicalIndex>> ranges = index.IndexRanges(q).ranges;
        std::vector<Range<PhysicalIndex>> want = {{9, 10}};
        EXPECT_EQ(ranges.size(), want.size());
        for (size_t i = 0; i < want.size(); i++) {
            EXPECT_EQ(ranges[i].start, want[i].start);
            EXPECT_EQ(ranges[i].end, want[i].end);
        }
    }
    
    TEST_F(OutlierIndexTest, TestRangesNoMatch) {
        auto pts = ValuesToPoints({10, 6, 6, 7, 8, 2, 3, 5, 1, 2, 9, 3});
        std::vector<size_t> outlier_list = {3, 5, 6, 7, 9};
        OutlierIndex<TESTD> index(outlier_list);
        index.SetIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));
        index.SetOutlierIndexer(std::make_unique<PrimaryBTreeIndex<TESTD>>(0, 1));

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


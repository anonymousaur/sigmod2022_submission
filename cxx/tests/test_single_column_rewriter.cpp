#include "gtest/gtest.h"
#include "single_column_rewriter.h"

#include "utils.h"
#include "types.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

namespace test {
    const size_t TESTD = 2;
    class SingleColumnRewriterTest : public ::testing::Test {
        
        public:
        
        std::string TestingFile() {
            std::string fname = "testing.tmp";
            std::ofstream file;
            file.open(fname, std::ios::trunc | std::ios::out);
            file << "testing" << std::endl;
            file.close();
            return fname;
        }

        // Takes in a vector of pairs of (mapped bucket range) --> (lists of target ranges)
        std::string ContinuousFile(std::vector<std::pair<ScalarRange, std::vector<ScalarRange>>>& spec) {
            std::string fname = "testing.tmp";
            std::ofstream file;
            file.open(fname, std::ios::trunc | std::ios::out);
            file << "continuous" << std::endl << "0 1" << std::endl;
            for (const auto& mapping : spec) {
                ScalarRange mapped_bucket = mapping.first;
                file << mapped_bucket.first << " " << mapped_bucket.second
                    << " " << mapping.second.size() << std::endl;
                for (const auto& sr : mapping.second) {
                    file << sr.first << " " << sr.second << std::endl;
                }
            }
            file.close();
            return fname; 
        }

        std::string CategoricalFile(std::vector<std::pair<Scalar, std::vector<Scalar>>>& spec) {
            std::string fname = "testing.tmp";
            std::ofstream file;
            file.open(fname, std::ios::trunc | std::ios::out);
            file << "categorical" << std::endl << "0 1" << std::endl;
            for (const auto& mapping : spec) {
                file << mapping.first << " " << mapping.second.size() << std::endl;
                for (Scalar s : mapping.second) {
                    file << s << " ";
                }
                file << std::endl;
            }
            file.close();
            return fname;
        }

        bool QueryFiltersEqual(const QueryFilter& got, const QueryFilter& want) {
            bool match = (got.present == want.present);
            if (match) {
                match &= got.is_range == want.is_range;
            }
            if (match) {
                if (got.is_range) {
                    match &= ArrayEqual(got.ranges, want.ranges);
                } else {
                    match &= ArrayEqual(got.values, want.values);
                }
            }
            if (!match) {
                std::cout << "Query filter mismatch: got " << std::endl
                    << QueryFilterToString(got) << std::endl
                    << " but want "
                    << QueryFilterToString(want) << std::endl;
            }
            return match;
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

    TEST_F(SingleColumnRewriterTest, TestUnionSortedRanges) {
        std::vector<ScalarRange> range1 = {{0, 10}, {20, 30}, {35, 40}};
        std::vector<ScalarRange> range2 = {{5, 15}, {25, 35}, {55, 65}};
        SingleColumnRewriter<TESTD> rewriter(TestingFile());

        auto got = rewriter.UnionSortedRanges(range1, range2);
        std::vector<ScalarRange> want = {{0, 15}, {20, 40}, {55, 65}};
        EXPECT_TRUE(ArrayEqual(got, want));
    }
    
    TEST_F(SingleColumnRewriterTest, TestIntersectSortedRanges) {
        std::vector<ScalarRange> range1 = {{0, 10}, {20, 30}, {35, 40}};
        std::vector<ScalarRange> range2 = {{5, 15}, {25, 35}, {55, 65}};
        SingleColumnRewriter<TESTD> rewriter(TestingFile());

        auto got = rewriter.IntersectSortedRanges(range1, range2);
        std::vector<ScalarRange> want = {{5, 10}, {25, 30}};
        EXPECT_TRUE(ArrayEqual(got, want));
    }

    TEST_F(SingleColumnRewriterTest, TestRewriteContinuousNoTargetFilter) {
        std::vector<std::pair<ScalarRange, std::vector<ScalarRange>>> spec =
            {{{0, 5}, {{0, 3}, {6, 10}}},
             {{5, 10}, {{0, 5}, {7, 11}}},
             {{10, 15}, {{10, 15}, {18, 25}}},
             {{15, 20}, {{5, 10}, {15, 20}}}};

        SingleColumnRewriter<TESTD> rewriter(ContinuousFile(spec));
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = true, .ranges = {{0, 8}}, .values = {}};

        rewriter.Rewrite(q);
        QueryFilter want = {.present = true, .is_range = true, .ranges = {{0, 5}, {6, 11}}};
        EXPECT_TRUE(QueryFiltersEqual(q.filters[1], want));
        EXPECT_TRUE(QueryFiltersEqual(q.filters[0], q.filters[0]));        
    }
    
    TEST_F(SingleColumnRewriterTest, TestRewriteContinuousWithTargetFilter) {
        std::vector<std::pair<ScalarRange, std::vector<ScalarRange>>> spec =
            {{{0, 5}, {{0, 3}, {6, 10}}},
             {{5, 10}, {{0, 5}, {7, 11}}},
             {{10, 15}, {{10, 15}, {18, 25}}},
             {{15, 20}, {{5, 10}, {15, 20}}}};

        SingleColumnRewriter<TESTD> rewriter(ContinuousFile(spec));
        Query<TESTD> q;
        q.filters[1] = {.present = true, .is_range = true, .ranges = {{3, 8}}};
        q.filters[0] = {.present = true, .is_range = true, .ranges = {{0, 8}}, .values = {}};

        rewriter.Rewrite(q);
        QueryFilter want = {.present = true, .is_range = true, .ranges = {{3, 5}, {6, 8}}};
        EXPECT_TRUE(QueryFiltersEqual(q.filters[1], want));
        EXPECT_TRUE(QueryFiltersEqual(q.filters[0], q.filters[0]));        
    }
    
    TEST_F(SingleColumnRewriterTest, TestRewriteCategoricalNoTargetFilter) {
        std::vector<std::pair<Scalar, std::vector<Scalar>>> spec =
            {{0, {0, 1, 2, 6, 7, 8, 9}},
             {1, {0, 1, 2, 3, 4, 8, 9, 10}},
             {2, {2, 3, 5, 10, 11, 14, 15}},
             {3, {14, 15, 16, 17, 18}}};

        SingleColumnRewriter<TESTD> rewriter(CategoricalFile(spec));
        Query<TESTD> q;
        q.filters[1] = {.present = false};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {0, 1}};

        rewriter.Rewrite(q);
        // No requirement in code for these to be sorted.
        std::sort(q.filters[1].values.begin(), q.filters[1].values.end());
        QueryFilter want = {.present = true, .is_range = false, .values = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10}};
        EXPECT_TRUE(QueryFiltersEqual(q.filters[1], want));
        EXPECT_TRUE(QueryFiltersEqual(q.filters[0], q.filters[0]));        
    }
    
    TEST_F(SingleColumnRewriterTest, TestRewriteCategoricalWithTargetFilter) {
        std::vector<std::pair<Scalar, std::vector<Scalar>>> spec =
            {{0, {0, 1, 2, 6, 7, 8, 9}},
             {1, {0, 1, 2, 3, 4, 8, 9, 10}},
             {2, {2, 3, 5, 10, 11, 14, 15}},
             {3, {14, 15, 16, 17, 18}}};

        SingleColumnRewriter<TESTD> rewriter(CategoricalFile(spec));
        Query<TESTD> q;
        q.filters[1] = {.present = true, .is_range = false, .ranges = {}, .values = {2, 5, 9}};
        q.filters[0] = {.present = true, .is_range = false, .ranges = {}, .values = {0, 1}};

        rewriter.Rewrite(q);
        QueryFilter want = {.present = true, .is_range = false, .ranges = {}, .values = {2, 9}};
        EXPECT_TRUE(QueryFiltersEqual(q.filters[1], want));
        EXPECT_TRUE(QueryFiltersEqual(q.filters[0], q.filters[0]));        
    }
}

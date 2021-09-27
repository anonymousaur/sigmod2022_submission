#include "gtest/gtest.h"
#include "merge_utils.h"

#include <random>
#include <chrono>
#include <vector>
#include "utils.h"

using namespace std;

namespace test {

    const size_t TESTD = 2;
    class MergeUtilsTest : public ::testing::Test {
        public:
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

    TEST_F(MergeUtilsTest, TestIntersect) {
        std::vector<size_t> list1 = {0, 3, 5, 6, 7, 10, 20, 21, 23, 25};
        std::vector<size_t> list2 = {0, 1, 4, 5, 7, 8, 10};
        std::vector<size_t> list3 = {2, 6, 20, 21, 23, 25};

        std::vector<size_t> want12 = {0, 5, 7, 10};
        std::vector<size_t> want13 = {6, 20, 21, 23, 25};
        std::vector<size_t> want23 = {};

        auto got12 = MergeUtils::Intersect(list1, list2);
        auto got13 = MergeUtils::Intersect(list1, list3);
        auto got23 = MergeUtils::Intersect(list2, list3);
        // Make sure they work in reverse
        auto got21 = MergeUtils::Intersect(list2, list1);
        auto got31 = MergeUtils::Intersect(list3, list1);
        auto got32 = MergeUtils::Intersect(list3, list2);

        EXPECT_TRUE(ArrayEqual(got12, want12));
        EXPECT_TRUE(ArrayEqual(got21, want12));
        EXPECT_TRUE(ArrayEqual(got13, want13));
        EXPECT_TRUE(ArrayEqual(got31, want13));
        EXPECT_TRUE(ArrayEqual(got23, want23));
        EXPECT_TRUE(ArrayEqual(got32, want23));
    }

    TEST_F(MergeUtilsTest, TestIntersectPhysicalIndexSets) {
        Ranges<PhysicalIndex> r1 = {{5, 10}, {15, 20}, {50, 60}};
        List<PhysicalIndex> l1 = {2, 25, 26, 31, 36, 42};
        Ranges<PhysicalIndex> r2 = {{8, 18}, {25, 30}, {50, 55}};
        List<PhysicalIndex> l2 = {2, 5, 20, 42, 59, 60};

        Ranges<PhysicalIndex> want_range = {{8, 10}, {15, 18}, {50, 55}};
        List<PhysicalIndex> want_list = {2, 5, 25, 26, 42, 59};

        auto got_set = MergeUtils::Intersect<PhysicalIndex>({r1, l1}, {r2, l2});
        EXPECT_TRUE(ArrayEqual(got_set.ranges, want_range));
        EXPECT_TRUE(ArrayEqual(got_set.list, want_list));
    }

    TEST_F(MergeUtilsTest, TestUnionHeap) {
        List<PhysicalIndex> l1 = {1, 3, 5, 7};
        List<PhysicalIndex> l2 = {4, 6, 9, 19, 20};
        List<PhysicalIndex> l3 = {8, 12, 15, 16};
        
        List<PhysicalIndex> want = {1, 3, 4, 5, 6, 7, 8, 9, 12, 15, 16, 19, 20};
        List<PhysicalIndex> got = MergeUtils::Union<PhysicalIndex>({&l1, &l2, &l3});
        EXPECT_TRUE(ArrayEqual(got, want));
    }

    TEST_F(MergeUtilsTest, TestCoalesce) {
        std::vector<ScalarRange> ranges = {{0, 4}, {3, 6}, {8, 10}, {10, 12}, {15, 30}, {20, 25}, {24, 27}};
        std::vector<ScalarRange> want = {{0, 6}, {8, 12}, {15, 30}};
        auto got = MergeUtils::Coalesce(ranges.begin(), ranges.end());
        EXPECT_TRUE(ArrayEqual(got, want));
    }

    TEST_F(MergeUtilsTest, TimedTest) {
        std::default_random_engine gen;
        std::uniform_int_distribution<int> dist(1,1<<30);
        std::vector<const List<PhysicalIndex> *> ix_lsts;
        std::vector<size_t> for_sorting;
        for (size_t i = 0; i < 10; i++) {
            size_t n = 1000000;
            auto v = new std::vector<size_t>();
            v->reserve(n);
            for (size_t j = 0; j < n; j++) {
                v->push_back(dist(gen));
                for_sorting.push_back(dist(gen));
            }
            std::sort(v->begin(), v->end());
            ix_lsts.push_back(v);
        }
                
        auto start = std::chrono::high_resolution_clock::now();
        List<PhysicalIndex> got = MergeUtils::Union(ix_lsts);
        auto finish = std::chrono::high_resolution_clock::now();
        auto tt = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
        std::cout << "Result size " << got.size() << ", time = " << tt / 1e3 << "us" << std::endl;
        
        start = std::chrono::high_resolution_clock::now();
        std::sort(for_sorting.begin(), for_sorting.end());
        finish = std::chrono::high_resolution_clock::now();
        tt = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
        std::cout << "Result size " << for_sorting.size() << ", time = " << tt / 1e3 << "us" << std::endl;
        
        
        for (auto v : ix_lsts) {
            delete v;
        }
        ix_lsts.clear();
        for (size_t i = 0; i < 100; i++) {
            size_t n = 100000;
            auto v = new std::vector<size_t>();
            v->reserve(n);
            for (size_t j = 0; j < n; j++) {
                v->push_back(dist(gen));
            }
            std::sort(v->begin(), v->end());
            ix_lsts.push_back(v);
        }
        start = std::chrono::high_resolution_clock::now();
        got = MergeUtils::Union(ix_lsts);
        finish = std::chrono::high_resolution_clock::now();
        tt = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
        std::cout << "Result size " << got.size() << ", time = " << tt / 1e3 << "us" << std::endl;
    }


} // namespace test


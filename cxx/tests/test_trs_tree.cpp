#include "gtest/gtest.h"
#include "trs_node.h"

#include <random>
#include <chrono>
#include <vector>
#include "utils.h"

using namespace std;

namespace test {

    class TRSNodeTest : public ::testing::Test {
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
       
        template <typename T> 
        bool ArraysNearlyEqual(const std::vector<T>& got, const std::vector<T>& want) {
            bool match = (got.size() == want.size());
            if (match) {
                for (size_t i = 0; i < got.size(); i++) {
                    match &= fabs(got[i] - want[i]) < 1e-7;
                }
            }
            if (!match) {
                std::cout << "Got " << VectorToString(got) << " but wanted "
                   << VectorToString(want) << std::endl; 
            }
            return match;
        }
    };

    TEST_F(TRSNodeTest, TestBuild) {
        std::vector<Scalar> xs(50, 0);
        std::iota(xs.begin(), xs.end(), -10);
        auto ys = xs;
        std::cout << "Building with size: " << xs.size() << std::endl;
        // Don't allow more than one layer depth.
        auto tn = TRSNode(2, 0.1, 8, 0, 0);
        tn.Build(xs, ys, 0, xs.size());
        
        EXPECT_TRUE(fabs(tn.slope - 1) < 1e-7);
        EXPECT_TRUE(fabs(tn.intercept) < 1e-7);
        EXPECT_TRUE(tn.children.size() == 0);
        EXPECT_TRUE(tn.bounds.first == -10 && tn.bounds.second == 39);
    }
    
    TEST_F(TRSNodeTest, TestLookup) {
        std::vector<Scalar> xs(50, 0);
        std::iota(xs.begin(), xs.end(), -10);
        auto ys = xs;
        // Don't allow more than one layer depth.
        auto tn = TRSNode(2, 0.1, 8, 0, 0);
        tn.Build(xs, ys, 0, xs.size());
        
        std::vector<ScalarRange> ranges;
        tn.Lookup({0, 5}, &ranges);
        EXPECT_TRUE(ranges.size() == 1);
        EXPECT_TRUE(fabs(ranges[0].first) < 1e-7 && fabs(ranges[0].second-5) < 1e-7);

        ranges.clear();
        tn.Lookup({100, 200}, &ranges);
        EXPECT_TRUE(ranges.empty());

        // The tolerance is 49 * 2 / (2*50) = 0.98
        EXPECT_NEAR(tn.tolerance, 0.98, 1e-7);
        tn.Lookup({30, 70}, &ranges);
        EXPECT_TRUE(ranges.size() == 1);
        std::cout << ranges[0] << std::endl;
       
        // The range is [30 - tolerance, 39+tolerance], truncated. 
        EXPECT_TRUE(fabs(ranges[0].first-29) < 1e-7 && fabs(ranges[0].second-39) < 1e-7);
    }

    TEST_F(TRSNodeTest, TestChildren) {
        std::vector<Scalar> xs(50, 0);
        std::iota(xs.begin(), xs.end(), 0);
        std::vector<Scalar> ys;
        for (size_t i = 0; i < 25; i++) {
            ys.push_back(xs[i]);
        }
        for (size_t i = 25; i < xs.size(); i++) {
            ys.push_back(xs[i] - 25);
        }
        auto tn = TRSNode(2, 0.1, 2, 0, 1);
        tn.Build(xs, ys, 0, xs.size());

        EXPECT_EQ(tn.children.size(), 2);
        EXPECT_EQ(tn.children[0]->bounds, ScalarRange(0, 24));
        EXPECT_EQ(tn.children[1]->bounds, ScalarRange(25, 49));
        EXPECT_EQ(tn.bounds, ScalarRange(0, 49));

        EXPECT_NEAR(tn.children[0]->slope, 1, 1e-7);
        EXPECT_NEAR(tn.children[0]->intercept, 0, 1e-7);
        EXPECT_NEAR(tn.children[1]->slope, 1, 1e-7);
        EXPECT_NEAR(tn.children[1]->intercept, -25, 1e-7);
    }

    TEST_F(TRSNodeTest, TestChildrenLookup) {
        std::vector<Scalar> xs(50, 0);
        std::iota(xs.begin(), xs.end(), 0);
        std::vector<Scalar> ys;
        for (size_t i = 0; i < 25; i++) {
            ys.push_back(xs[i]);
        }
        for (size_t i = 25; i < xs.size(); i++) {
            ys.push_back(xs[i] - 25);
        }
        auto tn = TRSNode(2, 0.1, 2, 0, 1);
        tn.Build(xs, ys, 0, xs.size());

        std::vector<ScalarRange> ranges;
        tn.Lookup({20, 30}, &ranges);
        EXPECT_EQ(ranges.size(), 2);
        // The tolerance is 0.96, so the lower range is -0.96, which truncates to 0
        EXPECT_TRUE(fabs(ranges[0].first) < 1e-7);
        EXPECT_TRUE(fabs(ranges[0].second - 5) < 1e-7);
        EXPECT_TRUE(fabs(ranges[1].first - 19) < 1e-7);
        EXPECT_TRUE(fabs(ranges[1].second - 24) < 1e-7); 
    }

    TEST_F(TRSNodeTest, TestNegativeSlope) {
        std::vector<Scalar> xs(50, 0);
        std::iota(xs.begin(), xs.end(), 0);
        std::vector<Scalar> ys;
        for (size_t i = 0; i < 25; i++) {
            ys.push_back(50 - xs[i]);
        }
        for (size_t i = 25; i < xs.size(); i++) {
            ys.push_back(75 - xs[i]);
        }
        auto tn = TRSNode(2, 0.1, 2, 0, 1);
        tn.Build(xs, ys, 0, xs.size());
        EXPECT_EQ(tn.children.size(), 2);
        EXPECT_EQ(tn.children[0]->bounds, ScalarRange(0, 24));
        EXPECT_EQ(tn.children[1]->bounds, ScalarRange(25, 49));
        EXPECT_EQ(tn.bounds, ScalarRange(0, 49));

        EXPECT_NEAR(tn.children[0]->slope, -1, 1e-7);
        EXPECT_NEAR(tn.children[0]->intercept, 50, 1e-7);
        EXPECT_NEAR(tn.children[1]->slope, -1, 1e-7);
        EXPECT_NEAR(tn.children[1]->intercept, 75, 1e-7);

        std::vector<ScalarRange> ranges;
        tn.Lookup({20, 30}, &ranges);
        EXPECT_EQ(ranges.size(), 2);
        
        EXPECT_TRUE(fabs(ranges[0].first - 25) < 1e-7);
        EXPECT_TRUE(fabs(ranges[0].second - 30) < 1e-7);
        EXPECT_TRUE(fabs(ranges[1].first - 44) < 1e-7);
        EXPECT_TRUE(fabs(ranges[1].second - 50) < 1e-7); 
    }
};

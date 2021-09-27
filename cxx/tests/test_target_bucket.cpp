#include "gtest/gtest.h"
#include "target_bucket.h"

#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <vector>
#include "utils.h"

using namespace std;

namespace test {

    class TargetBucketTest : public ::testing::Test {
        public:
        template <typename T>
        bool ArrayEqual(const std::vector<T>& got, const std::vector<T>& want) {
            bool match = (got.size() == want.size());
            if (match) {
                for (size_t i = 0; i < got.size(); i++) {
                    match &= (got[i] == want[i]);
                    if (got[i] != want[i]) {
                        std::cout << "Index " << i << ": got {"
                            << got[i].first << ", " << got[i].second  << "} but wanted {"
                            << want[i].first << ", " << want[i].second << "}" << std::endl;
                    }
                }
            }
            return match;
        }
        
        void DumpBucket(const TargetBucket& tb) {
            std::cerr << "Target Bucket {" << std::endl
                << "  num_inlier_points = " << tb.num_inlier_points_ << std::endl
                << "  num_inlier_buckets = " << tb.num_inlier_buckets_ << std::endl
                << "  buckets = {" << std::endl;
            for (const auto& mb : tb.buckets_) {
                std::cerr << "    {index = " << mb.index
                   << ", num_points = " << mb.num_points
                   << ", outlier = " << mb.is_outlier << "}" << std::endl; 
            }
            std::cerr << "}" << std::endl;
        }

        bool CheckConsistency(const TargetBucket& tb) {
            bool chk = tb.Consistent();
            if (!chk) {
                DumpBucket(tb);
            }
            return chk;
        }

        bool BucketMatches(const TargetBucket& tb,
                int32_t num_inlier_points,
                int32_t num_inlier_buckets) {
            bool match = tb.num_inlier_points_ == num_inlier_points &&
                    tb.num_inlier_buckets_ == num_inlier_buckets;
            if (!match) {
               DumpBucket(tb);
            }
            return match; 
        }
    };

    TEST_F(TargetBucketTest, TestBuildFromScratch) {
        TargetBucket tb(5.0, 0);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 0, 0));

        tb.AddPoints(5, 10);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 10, 1));

        tb.AddPoints(6, 12);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 22, 2));

        tb.AddPoints(9, 1);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 22, 2));

        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_OUTLIER}};

        EXPECT_TRUE(ArrayEqual(got, want));
    }

    TEST_F(TargetBucketTest, TestAddPointsNoChange) {
        TargetBucket tb(5.0, 0);
        tb.AddPoints(5, 10);
        tb.AddPoints(6, 12);
        tb.AddPoints(9, 1);
        
        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_OUTLIER}};
        EXPECT_TRUE(ArrayEqual(got, want));
        tb.ResetDiffs();

        tb.AddPoints(9, 3);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 22, 2));
        EXPECT_TRUE(ArrayEqual(tb.Diffs(), {{9, REMAIN_OUTLIER}}));
        tb.ResetDiffs();
            
        tb.AddPoints(6, 4);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 26, 2));
        EXPECT_TRUE(ArrayEqual(tb.Diffs(), {{6, REMAIN_INLIER}}));
        tb.ResetDiffs();
        
        tb.AddPoints(9, 4);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 26, 2));
        EXPECT_TRUE(ArrayEqual(tb.Diffs(), {{9, REMAIN_OUTLIER}})); 
    }

    TEST_F(TargetBucketTest, TestAddPointsChangeToInlier) {
        TargetBucket tb(5.0, 0);
        tb.AddPoints(5, 10);
        tb.AddPoints(6, 12);
        tb.AddPoints(9, 1);

        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_OUTLIER}};
        EXPECT_TRUE(ArrayEqual(got, want));
        tb.ResetDiffs();
        
        // Bucket 9 should convert to inlier
        tb.AddPoints(9, 7);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 30, 3));
        EXPECT_TRUE(ArrayEqual(tb.Diffs(), {{9, OUTLIER_TO_INLIER}}));
    }

    TEST_F(TargetBucketTest, TestAddPointsChangeToOutlier) {
        TargetBucket tb(5.0, 0);
        tb.AddPoints(5, 10);
        tb.AddPoints(6, 12);
        tb.AddPoints(9, 8);
        
        EXPECT_TRUE(BucketMatches(tb, 30, 3));
        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_INLIER}};
        EXPECT_TRUE(ArrayEqual(got, want));
        tb.ResetDiffs();

        // Bucket 9 should convert back to an outlier.
        tb.AddPoints(5, 5);
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 27, 2));
        got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        EXPECT_TRUE(ArrayEqual(tb.Diffs(), {{5, REMAIN_INLIER}, {9, INLIER_TO_OUTLIER}}));
    }

    TEST_F(TargetBucketTest, TestAddPointsBatchNoOutliers) {
        TargetBucket tb(5.0, 0);
        tb.AddPointsBatch({std::make_pair<int32_t, int32_t>(5, 10),
                std::make_pair<int32_t, int32_t>(6, 12),
                std::make_pair<int32_t, int32_t>(9, 8)});
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 30, 3));
        
        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_INLIER}};
        EXPECT_TRUE(ArrayEqual(got, want));
        tb.ResetDiffs();
    }
    
    TEST_F(TargetBucketTest, TestAddPointsBatchWithOutliers) {
        TargetBucket tb(5.0, 0);
        tb.AddPointsBatch({std::make_pair<int32_t, int32_t>(5, 10),
                std::make_pair<int32_t, int32_t>(6, 12),
                std::make_pair<int32_t, int32_t>(9, 2)});
        EXPECT_TRUE(CheckConsistency(tb));
        EXPECT_TRUE(BucketMatches(tb, 22, 2));
        auto got = tb.Diffs();
        std::sort(got.begin(), got.end(), [] (const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
        std::vector<std::pair<int32_t, DiffType>> want =
                {{5, NEW_INLIER}, {6, NEW_INLIER}, {9, NEW_OUTLIER}};
        EXPECT_TRUE(ArrayEqual(got, want));
        tb.ResetDiffs();
    }
};


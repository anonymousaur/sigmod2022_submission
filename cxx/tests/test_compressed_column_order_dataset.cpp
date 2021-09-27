#include "gtest/gtest.h"
#include "compressed_column_order_dataset.h"
#include <vector>
#include <array>
#include <bitset>

using namespace std;

namespace test {

    typedef std::vector<Point<1>> Column;
    const size_t TEST_DIM = 1;
    class CompressedColumnDatasetTest : public ::testing::Test {
      protected:
        Column GenBlockData(Scalar offset, Scalar max_diff, size_t n) {
            std::vector<Point<TEST_DIM>> data;
            for(size_t i = 0; i < n; i++) {
                Scalar delta = rand() % max_diff;
                data.push_back({offset + delta});
            }
            return data;
        }

        vector<bool> GenPresentBits(size_t size, vector<size_t> deleted) {
            std::vector<bool> bits(size, true);
            for (size_t ix : deleted) {
                bits[ix] = 0;
            }
            return bits;
        }
    };

    TEST_F(CompressedColumnDatasetTest, TestCorrectBitWidthSmall) {
        Column data = GenBlockData(1000, 7, 50);
        // To make the result deterministic, add one value with delta 7.
        data.push_back({1007});
        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        // Since the max diff is 7, at most 3 bits get used.
        EXPECT_EQ(3, dset.cblocks_[0].bit_width);

        data = GenBlockData(1000, 12, 50);
        data.push_back({1012});
        CompressedColumnOrderDataset<TEST_DIM> dset2(data);
        // Since the max diff is 7, at most 3 bits get used.
        EXPECT_EQ(4, dset2.cblocks_[0].bit_width);
    }

    TEST_F(CompressedColumnDatasetTest, TestAccessSingleBlock) {
        size_t size = 50;
        Column data = GenBlockData(1000, 12, size);
        CompressedColumnOrderDataset<TEST_DIM> dset(data);

        for (size_t i = 0; i < size; i++) {
            EXPECT_EQ(data[i][0], dset.GetCoord(i, 0));
        }
    }
    
    TEST_F(CompressedColumnDatasetTest, TestAccessManyBlocks) {
        size_t size = 1000;
        Column data = GenBlockData(1000, 12, size);
        CompressedColumnOrderDataset<TEST_DIM> dset(data);

        for (size_t i = 0; i < size; i++) {
            EXPECT_EQ(data[i][0], dset.GetCoord(i, 0));
        }
    }
    
    TEST_F(CompressedColumnDatasetTest, TestAccessManyBlocks_largeBitWidth) {
        size_t size = 1000;
        Column data = GenBlockData(1000, 1UL<<50, size);
        CompressedColumnOrderDataset<TEST_DIM> dset(data);

        for (size_t i = 0; i < size; i++) {
            EXPECT_EQ(data[i][0], dset.GetCoord(i, 0));
        }
    }
    
    TEST_F(CompressedColumnDatasetTest, TestAccessLarge_width3) {
        size_t size = 10000;
        Column data = GenBlockData(1000, 7, size);
        CompressedColumnOrderDataset<TEST_DIM> dset(data);

        for (size_t i = 0; i < size; i++) {
            EXPECT_EQ(data[i][0], dset.GetCoord(i, 0));
        }
    }
    
    TEST_F(CompressedColumnDatasetTest, TestAccessLarge_width7) {
        size_t size = 10000;
        Column data = GenBlockData(1000, 100, size);
        CompressedColumnOrderDataset<TEST_DIM> dset(data);

        for (size_t i = 0; i < size; i++) {
            EXPECT_EQ(data[i][0], dset.GetCoord(i, 0));
        }
    }

    TEST_F(CompressedColumnDatasetTest, TestGetCoordRange_full) {
        size_t size = 100;
        Column data = GenBlockData(1000, 1000, size);

        // start_ix + 64 should be < 100
        size_t start_ix = 22;
        Scalar range_low = 1200;
        Scalar range_high = 1500;
        vector<bool> valid;
        for (size_t i = start_ix; i < start_ix + 64UL; i++) {
            Scalar val = data[i][0];
            valid.push_back(val >= range_low && val <= range_high);
        }

        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        uint64_t got = dset.GetCoordRange(start_ix, start_ix + 64UL, 0, range_low, range_high);
        for (size_t i = 0; i < 64UL; i++) {
            bool got_bool = (got & (1UL << (63-i))) > 0;
            EXPECT_EQ(valid[i], got_bool);
        }
    }

    TEST_F(CompressedColumnDatasetTest, TestGetCoordRange_partial) {
        size_t size = 100;
        Column data = GenBlockData(1000, 1000, size);

        // start_ix + 64 should be < 100
        size_t start_ix = 22;
        size_t len = 30;
        Scalar range_low = 1200;
        Scalar range_high = 1500;
        vector<bool> valid;
        for (size_t i = start_ix; i < start_ix + len; i++) {
            Scalar val = data[i][0];
            valid.push_back(val >= range_low && val <= range_high);
        }

        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        uint64_t got = dset.GetCoordRange(start_ix, start_ix + len, 0, range_low, range_high);
        for (size_t i = 0; i < len; i++) {
            bool got_bool = (got & (1UL << i)) > 0;
            EXPECT_EQ(valid[len-1-i], got_bool);
        }
    }

    TEST_F(CompressedColumnDatasetTest, TestGetRangeValues_full) {
        size_t size = 100;
        Column data = GenBlockData(1000, 1000, size);

        // start_ix + 64 should be < 100
        size_t start_ix = 22;
        size_t len = 64;
        uint64_t valids = 0b100101111011001011011100111110111011111111110111111011100000001;
        vector<Scalar> values;
        for (size_t i = start_ix; i < start_ix + len; i++) {
            if (valids & 1UL << (63 - (i - start_ix))) {
                values.push_back(data[i][0]);
            }
        }

        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        vector<Scalar> got;
        dset.GetRangeValues(start_ix, start_ix + len, 0, valids, &got);
        EXPECT_EQ(values, got);
    }

    TEST_F(CompressedColumnDatasetTest, TestGetRangeSum) {
        size_t size = 100;
        Column data = GenBlockData(1000, 1000, size);

        // start_ix + 64 should be < 100
        size_t start_ix = 22;
        size_t len = 64;
        uint64_t valids = 0b1001011110110010110111001111101110111111111101111110111000000001;
        Scalar want = 0;
        for (size_t i = start_ix; i < start_ix + len; i++) {
            if (valids & (1UL << (63 - (i - start_ix)))) {
                want += data[i][0];
            }
        }

        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        Scalar got = dset.GetRangeSum(start_ix, start_ix + len, 0, valids);
        EXPECT_EQ(want, got);
    }
    
    TEST_F(CompressedColumnDatasetTest, TestGetRangeSum_multipleBlocks) {
        size_t size = 200;
        Column data = GenBlockData(1000, 1000, size);

        // start_ix + 64 should be < 100
        size_t start_ix = 100;
        size_t len = 64;
        uint64_t valids = 0b1001011110110010110111001111101110111111111101111110111000000001;
        Scalar want = 0;
        for (size_t i = start_ix; i < start_ix + len; i++) {
            if (valids & (1UL << (63 - (i - start_ix)))) {
                want += data[i][0];
            }
        }

        CompressedColumnOrderDataset<TEST_DIM> dset(data);
        Scalar got = dset.GetRangeSum(start_ix, start_ix + len, 0, valids);
        EXPECT_EQ(want, got);
    }

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


#!/bin/bash
set -x
BASE_DIR=/home/ubuntu/correlations/continuous/chicago_taxi


./run_mapped_correlation_index.sh 9 base \
    $BASE_DIR/chicago_taxi_sort_octree_0_2_8.bin \
    $BASE_DIR/queries_3_s01.dat \
    $BASE_DIR/index_0_2_8_a0.build


#./run_mapped_correlation_index.sh 9 chicago_taxi_corrix_0_2_3_a0.2_q4_s01 \
#    $BASE_DIR/chicago_taxi_sort_octree_0_2_3.bin \
#    $BASE_DIR/queries_4_s01.dat \
#    $BASE_DIR/index_0_2_3_a0.2.build > results/raw_chicago_taxi_corrix_0_2_3_a0.2_q4_s01.out
#
#./run_mapped_correlation_index.sh 9 chicago_taxi_corrix_0_2_3_a0.2_q8_s01 \
#    $BASE_DIR/chicago_taxi_sort_octree_0_2_3.bin \
#    $BASE_DIR/queries_8_s01.dat \
#    $BASE_DIR/index_0_2_3_a0.2.build > results/raw_chicago_taxi_corrix_0_2_3_a0.2_q8_s01.out


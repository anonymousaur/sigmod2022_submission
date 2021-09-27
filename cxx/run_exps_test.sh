#!/bin/bash
set -x
BASE_DIR=/home/ubuntu/correlations/continuous/wise

./run_mapped_correlation_index.sh 15 octree_base_run \
    $BASE_DIR/wise_sort_btree9.bin \
    $BASE_DIR/../stocks/queries_2_3_4_5_s0001.dat \
    $BASE_DIR/index_octree_base.build


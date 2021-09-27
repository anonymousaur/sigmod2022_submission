#!/bin/bash

BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/measure_beta
PUNCHLINE_DIR=/home/ubuntu/correlations/paper/experiments/punchline
queries=$EXP_DIR/queries/measure_beta_stocks_queries.dat
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

binary=/home/ubuntu/correlations/continuous/stocks/stocks_data_sort_flood_0_4.bin

$BINARY_DIR/run_mapped_correlation_index.sh 7 measure_beta_stocks_flood \
    $binary \
    $EXP_DIR/queries/measure_beta_stocks_queries.dat \
    $EXP_DIR/indexes/index_measure_beta_stocks.build

#binary=/home/ubuntu/correlations/continuous/chicago_taxi/chicago_taxi_sort_primary_8.bin
#
#$BINARY_DIR/run_mapped_correlation_index.sh 9 measure_beta_chicago_taxi \
#    $binary \
#    $PUNCHLINE_DIR/chicago_taxi/queries/queries_4_5_s01.dat \
#    $EXP_DIR/indexes/index_measure_beta_chicago_taxi.build

#binary=/home/ubuntu/correlations/continuous/wise/wise_sort_btree9.bin
#
#$BINARY_DIR/run_mapped_correlation_index.sh 15 measure_beta_wise \
#    $binary \
#    $PUNCHLINE_DIR/wise/queries/queries_10_11_12_s01.dat \
#    $EXP_DIR/indexes/index_measure_beta_wise.build


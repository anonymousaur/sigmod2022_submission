#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/stocks
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/stocks
queries=$BASE_DIR/queries_5_s05.dat
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

binary=$BASE_DIR/stocks_data_sort_btree4.bin

$BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_measure_beta \
    $binary \
    $queries \
    $EXP_DIR/indexes/index_measure_beta.build


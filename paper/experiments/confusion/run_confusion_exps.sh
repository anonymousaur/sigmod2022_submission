#!/bin/bash
set -x

EXP_DIR=/home/ubuntu/correlations/paper/experiments/confusion
BASE_DIR=/home/ubuntu/correlations/continuous/chicago_taxi
BINARY_DIR=/home/ubuntu/correlations/cxx
TUNING_FILE=${EXP_DIR}/stats.json

ALPHAS=( 1 )
queries=(
    $EXP_DIR/queries/queries_1_3_4_5_s001.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

suffix=flood_0_2_8
dataset=${BASE_DIR}/chicago_taxi_sort_${suffix}.bin
#METHODS=( "isoforest" "dbscan" )
METHODS=( "dbscan" )
for method in "${METHODS[@]}"
do
    # Do these first so we can confirm if anything is wrong 
    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_outliers_${method}_a${a}_${suffix} \
            $dataset \
            $all_queries \
            $EXP_DIR/indexes/index_outliers_${method}_a${a}_${suffix}.build
    done
done

suffix=octree_0_2_8_p10000
dataset=${BASE_DIR}/chicago_taxi_sort_${suffix}.bin
METHODS=( "dbscan" )
#METHODS=( "isoforest" "dbscan" )

for method in "${METHODS[@]}"
do
    # Do these first so we can confirm if anything is wrong 
    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_outliers_${method}_a${a}_${suffix} \
            $dataset \
            $all_queries \
            $EXP_DIR/indexes/index_outliers_${method}_a${a}_${suffix}.build
    done
done



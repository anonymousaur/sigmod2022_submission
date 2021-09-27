#!/bin/bash
set -x

EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/stocks
BASE_DIR=/home/ubuntu/correlations/continuous/stocks
BINARY_DIR=/home/ubuntu/correlations/cxx
TUNING_FILE=${EXP_DIR}/stats.json

ALPHAS=( 0 0.2 0.5 1 2 5 )
queries=( $EXP_DIR/queries/queries_2_3_5_s0001.dat 
    $EXP_DIR/queries/queries_2_3_5_s001.dat
    $EXP_DIR/queries/queries_2_3_5_s01.dat
    $EXP_DIR/queries/queries_2_3_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

suffix=flood_0_4
dataset=$BASE_DIR/stocks_data_sort_${suffix}.bin

if [ ! -f "$dataset" ]
then
    echo "Could not find flood sorted data. Regenerating..."
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_flood_test \
        $BASE_DIR/stocks_data_sort_btree4.bin \
        $EXP_DIR/queries/queries_0_4_s01.dat \
        $EXP_DIR/indexes/index_flood.build

    mv flood_sorted_points.bin $dataset
    python ../continuous/generate_target_buckets.py flood_sorted_buckets.dat $BASE_DIR/target_buckets_$suffix.bin 

    if [ $? -ne 0 ];
    then
        echo "Error generating buckets. Aborting..."
        exit 1
    fi

    if [ ! -f "$dataset" ]
    then
        echo "Dataset not generated. Aborting..."
        exit 1
    fi
fi

if [ ! -f "$TUNING_FILE" ];
then
    mbname="measure_beta_stocks_flood"
    $BINARY_DIR/run_mapped_correlation_index.sh 7 $mbname \
        $dataset \
        $EXP_DIR/queries/measure_beta_stocks_queries.dat \
        $EXP_DIR/indexes/index_measure_beta_stocks.build

    python $EXP_DIR/../../measure_beta/fit_performance_data.py \
        --raw-results $EXP_DIR/raw_results/$mbname.output \
        --output $TUNING_FILE

    if [ $? -ne 0 ];
    then
        echo "Stats generation failed. Aborting..."
        exit 1
    fi
fi

exit 0

if ! python $BASE_DIR/../validate_index_specs.py --dir indexes ;
then
    echo "Index specs are not ready. Running Cortex..."
    cd $BASE_DIR/..
    python run_stocks_data.py --suffix flood_0_4 --stats $TUNING_FILE
    cd -
fi

if ! python $BASE_DIR/../validate_index_specs.py --dir indexes ;
then
    echo "Index specs still incomplete. Aborting..."
    exit 1
fi

# Do these first so we can confirm if anything is wrong 
for a in "${ALPHAS[@]}"
do
    # Ours
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_a${a}_${suffix} \
        $dataset \
        $all_queries \
        $EXP_DIR/indexes/index_a${a}_${suffix}.build
done

$BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_secondary_${suffix} \
    $dataset \
    $all_queries \
    $EXP_DIR/indexes/index_secondary_${suffix}.build

$BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_hermit_${suffix} \
    $dataset \
    $all_queries \
    $EXP_DIR/indexes/index_hermit_${suffix}.build

$BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_cm_${suffix} \
    $dataset \
    $all_queries \
    $EXP_DIR/indexes/index_cm_${suffix}.build



#!/bin/bash

DIM=$1
NAME=$2
DATASET=$3
WORKLOAD=$4
SPEC=$5
REWRITER=$6

if [ -z "$REWRITER" ];
then
    echo "Requires 6 arguments"
    exit 0
fi

savefile=results/$NAME.dat
if [ -f $savefile ];
then
    echo "Results file $savefile already exists. Skipping"
    exit 0
fi

set -x

/home/ubuntu/correlations/cxx/build${DIM}/run_linear_correlation_index \
    --name=$NAME \
    --dataset=$DATASET \
    --workload=$WORKLOAD \
    --visitor=index \
    --rewriter=$REWRITER \
    --indexer-spec=$SPEC \
    --save=results/$NAME.dat > raw_results/$NAME.output

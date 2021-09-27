#!/bin/bash

DIM=$1
NAME=$2
DATASET=$3
BASE_COLS=$4
WORKLOAD=$5
SPEC=$6

if [ -z "$SPEC" ];
then
    echo "Requires 5 arguments"
    exit 0
fi

savefile=results/$NAME
if [ -f $savefile.dat ];
then
    echo "Results file $savefile.dat already exists. Skipping"
    exit 0
fi

# Make sure the directory is built
dir=/home/ubuntu/correlations/cxx/build$DIM
if [ ! -d "$d" ]
then
    mkdir -p $dir
    cd $dir
    cmake -DDIM=$DIM ..
    cd -
fi

cd $dir
make -j
cd -

set -x

$dir/run_mapped_correlation_index \
    --name=$NAME \
    --dataset=$DATASET \
    --workload=$WORKLOAD \
    --visitor=index \
    --indexer-spec=$SPEC \
    --base-cols=$BASE_COLS \
    --save=$savefile > raw_results/$NAME.output


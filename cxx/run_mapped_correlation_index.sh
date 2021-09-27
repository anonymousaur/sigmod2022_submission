#!/bin/bash

DIM=$1
NAME=$2
DATASET=$3
WORKLOAD=$4
SPEC=$5

if [ ! -f "$SPEC" ];
then
    echo "Requires 5 arguments"
    exit 0
fi

if [ ! -f "$DATASET" ];
then
    echo "Dataset not found"
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
    --save=$savefile > raw_results/$NAME.output


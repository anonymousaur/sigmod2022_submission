#!/bin/bash

ptsfile=$1
bucketsfile=$2
suffix=$3

if [ -z "$suffix" ]
then
    echo "Needs suffix"
    exit 1
fi

basedir=/home/ubuntu/correlations/continuous/stocks
mv $ptsfile $basedir/stocks_data_sort_$suffix.bin
python generate_target_buckets.py $bucketsfile $basedir/target_buckets_$suffix.bin 


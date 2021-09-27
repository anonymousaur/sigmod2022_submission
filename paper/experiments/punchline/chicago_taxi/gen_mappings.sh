#!/bin/bash

OUTPUT_DIR=/home/ubuntu/correlations/continuous/chicago_taxi
SCRIPT_DIR=/home/ubuntu/correlations/continuous
curdir=`pwd`

cd $SCRIPT_DIR
#python $SCRIPT_DIR/run_chicago_taxi_data.py primary_8
python $SCRIPT_DIR/run_chicago_taxi_data.py octree_0_2_8_p1000

cd $curdir

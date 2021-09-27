#!/bin/bash

OUTPUT_DIR=/home/ubuntu/correlations/continuous/stocks
SCRIPT_DIR=/home/ubuntu/correlations/continuous
curdir=`pwd`

cd $SCRIPT_DIR
#python $SCRIPT_DIR/run_stocks_data.py btree4
python $SCRIPT_DIR/run_stocks_data.py 0_4_p1000

cd $curdir

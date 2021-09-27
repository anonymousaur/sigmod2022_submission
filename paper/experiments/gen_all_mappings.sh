

curdir=`pwd`

cd punchline/stocks
./gen_mappings.sh
cd $curdir

cd punchline/chicago_taxi
./gen_mappings.sh
cd $curdir



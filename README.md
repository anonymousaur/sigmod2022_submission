# Overview
This repository is a code supplement for the Sigmod 2022 submission titled "Cortex: Harnessing Correlations to Boost Query Performance".

What this repository has:
- Source code (C++) and build scripts. This repository should have everything necessary to compile the source code successfully.
- Evaluation scripts to generate results. This repo does NOT contain the datasets we used for evaluation, due to size constraints. Therefore, the evaluation scripts are not expected to succeed.
- Results from some past runs. If examining results, please only consider the latest run for each index type.
- Plotting and table generation scripts

What this repository does not have:
- Datasets
- Implementation of Flood

## Compiling
Requirements: `cmake` (version >= 3.12)

You must compile a new set of binaries for each dataset dimension. For example, if you have datasets with 7 and 9 columns, you would create two different builds.

```
mkdir build9
cd build9
cmake -DDIM=9 ..
make -j
```

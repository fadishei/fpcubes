# Introduction

This is the source code developed for the experiments of the FPCubes algorithm proposed in this paper:

**Hamid Faidhsie, and Azadeh Soltani, "The Curse of Indecomposable Aggregates for Big Data Exploratory Analysis with A Case for Frequent Pattern Cubes", Journal of Supercomputing, Springer, DOI: 10.1007/s11227-019-03053-8**

Frequent pattern cubes refer to structures and algorithms that facilitate exploratory frequent pattern analysis on multidimensional big datasets. For more information, please refer to the published paper.

# Compiling

In order to compile the source code, simply use the make command:

    make clean
	make

Upon a successful build, you will have the `fpc` binary file which can be executed from the Linux command line. use the `-h` switch to see its possible command line arguments.

# Data format

The `fpc` command expects the multidimensional input data to be in a specific format. The data should be stored inside a filesystem tree with folders named after dimensions and dimension values. At the leaf folder a transactions.csv should exist. For an example, please see the `data/example` folder:

    tree data/example
    data/example
    └── device
        ├── 0
        │   └── gender
        │       ├── 0
        │       │   └── transactions.csv
        │       └── 1
        │           └── transactions.csv
        └── 1
            └── gender
                ├── 0
                │   └── transactions.csv
                └── 1
                    └── transactions.csv

Dimension values should be zero-indexed integers. The transaction.csv files contain transactions of that dimension with items separated by commas. Items are also zero-indexed integers:

    cat data/example/device/0/gender/0/transactions.csv
    0 2 3 5
    1 4 5
    1 4
    0 2 3 4
    3 5

Datasets used in the paper can be obtained from their official sources cited in the paper. However, they should be processed and converted to the above format. I will not upload these large converted files here but you can find the Python scripts developed for the conversion in `scripts` folder.

# Usage

the `fpc` command will preprocess (premine) the input dataset and when finished, starts accepting exploratory queries from standard input using a specific format. Please see query folder for some examples. You can give multiple queries by feeding multiple lines to the program. Some examples of executing this command are given in the following:

Example 1 - run a single query and print frequent patterns:

    echo "/device/0/gender/0-1" | ./fpc -d data/example -m 0.27 -p
    # premine: t=0.000060,m=12804096,ands=45
    3 (8)
     5 (3)
    4 (6)
    2 (5)
     3 (5)
      4 (3)
     4 (3)
    1 (4)
    0 (3)

Example 2 - run a number of queries and print runtime statistics without printing frequent patterns:

    cat query/example.txt | ./fpc -d data/example -m 0.27 -H -s
    # premine: t=0.000060,m=12804096,ands=45
    query,cuboids,time,memory,fpcnt,fplenavg,ands,trancnt,tranlenavg,digs,leafdigs
    0,2,0.000023,12804096,11,1.545455,7,11,2.727273,14,7
    1,2,0.000009,12804096,8,1.250000,4,10,2.500000,10,5
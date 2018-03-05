#!/bin/bash

# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu

rm -rf lab2b_list.csv
touch lab2b_list.csv

# Mutex synchronized list operations, 1,000 iterations, 1,2,4,8,12,16,24 threads
# Spin-lock synchronized list operations, 1,000 iterations, 1,2,4,8,12,16,24 threads

for t in 1, 2, 4, 8, 12, 16, 24
do
    ./lab2_list --threads=$t --iterations=1000 --sync=m >> lab2b_list.csv
    ./lab2_list --threads=$t --iterations=1000 --sync=s >> lab2b_list.csv
done

# --yield=id, 4 lists, 1,4,8,12,16 threads, and 1, 2, 4, 8, 16 iterations (no synchronization)
# --yield=id, 4 lists, 1,4,8,12,16 threads, and 10, 20, 40, 80 iterations, with --sync=s and --sync=m

for t in 1, 4, 8, 12, 16
do
    for i in 1, 2, 4, 8, 16
    do
        ./lab2_list --threads=$t --lists=4 --iterations=$i --yield=id >> lab2b_list.csv  2> /dev/null
    done
    for i in 10, 20, 40, 80
    do
        ./lab2_list --threads=$t --lists=4 --iterations=$i --yield=id --sync=s >> lab2b_list.csv
        ./lab2_list --threads=$t --lists=4 --iterations=$i --yield=id --sync=m >> lab2b_list.csv
    done
done

# measure performance for both synchronized versions, without yields,
# for 1000 iterations, 1,2,4,8,12 threads, and 1,4,8,16 lists

for t in 1, 2, 4, 8, 12
do
    for l in 4, 8, 16
    do
        ./lab2_list --threads=$t --lists=$l --iterations=1000 --sync=s >> lab2b_list.csv
        ./lab2_list --threads=$t --lists=$l --iterations=1000 --sync=m >> lab2b_list.csv
    done
done

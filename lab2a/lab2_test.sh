#!/bin/bash

#NAME: Christopher Aziz
#EMAIL: caziz@ucla.edu
#ID: 304806012

rm -rf lab2_add.csv
rm -rf lab2_list.csv
touch lab2_add.csv
touch lab2_list.csv

for i in 10, 20, 40, 80, 100, 1000, 10000, 100000
do
    for t in 1, 2, 4, 8, 12
    do
        ./lab2_add --yield --iterations=$i --threads=$t  >> lab2_add.csv
        ./lab2_add --iterations=$i --threads=$t  >> lab2_add.csv
        ./lab2_add --iterations=$i --threads=$t  >> lab2_add.csv
    done
done

for i in 10, 20, 40, 80, 100, 1000, 10000
do
    for t in 1, 2, 4, 8, 12
    do
        ./lab2_add --sync=m --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --sync=s --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --sync=c --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=m --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=s --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=c --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=m --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=s --iterations=$i --threads=$t >> lab2_add.csv
        ./lab2_add --yield --sync=c --iterations=$i --threads=$t >> lab2_add.csv
    done
done


for i in 10, 100, 1000, 10000, 20000
do
    ./lab2_list --iterations=$i --threads=1 >> lab2_list.csv
done

for i in 1, 2, 4, 8, 16, 32
do
    for t in 1, 2, 4, 8, 12
    do
        ./lab2_list --yield=i --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --yield=d --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --yield=il --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --yield=dl --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=s --yield=i --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=s --yield=il --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=s --yield=d --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=s --yield=dl --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=m --yield=i --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=m --yield=il --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=m --yield=d --iterations=$i --threads=$t >> lab2_list.csv
        ./lab2_list --sync=m --yield=dl --iterations=$i --threads=$t >> lab2_list.csv
    done
done

for t in 1, 2, 4, 8, 12, 16, 24
do
    ./lab2_list --sync=m --iterations=1000 --threads=$t >> lab2_list.csv
    ./lab2_list --sync=s --iterations=1000 --threads=$t >> lab2_list.csv
done



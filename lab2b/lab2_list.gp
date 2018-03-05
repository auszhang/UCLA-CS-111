#! /usr/local/cs/bin/gnuplot

# NAME: Christopher Aziz
# EMAIL: caziz@ucla.edu

set terminal png
set datafile separator ","

set title "Throughput vs. Number of Threads for Synchronized List Operations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (op/s)"
set logscale y 10
set output 'lab2b_1.png'
set key right top

plot \
     "< grep list-none-m lab2b_list.csv | grep -E \",1000,1,\"" using ($2):(1000000000/($6)) \
	title 'mutex list, 1000 iterations' with linespoints lc rgb 'red', \
     "< grep list-none-s lab2b_list.csv | grep -E \",1000,1,\"" using ($2):(1000000000/($6)) \
	title 'spin-lock list, 1000 iterations' with linespoints lc rgb 'green'

set title "Average Time Per Op/Mutex Lock vs. Threads for Mutex List Ops"
set xlabel "Threads"
set logscale x 2
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep list-none-m lab2b_list.csv | grep 1000,1," using ($2):($7) \
	title 'average time per operation' with linespoints lc rgb 'red', \
     "< grep list-none-m lab2b_list.csv | grep 1000,1," using ($2):($8) \
	title 'average wait-for-mutex time' with linespoints lc rgb 'green'

set title "Successful Iterations vs. Threads for List Operations"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
"< grep list-id-m lab2b_list.csv" using ($2):($3) \
title 'mutex' with points lc rgb 'red', \
"< grep list-id-s lab2b_list.csv" using ($2):($3) \
title 'spin-lock' with points lc rgb 'green', \
"< grep list-id-none lab2b_list.csv" using ($2):($3) \
title 'no synchronization' with points lc rgb 'blue'

set title "Throughput vs. Number of Threads for Mutex Sub-Lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (op/s)"
set output 'lab2b_4.png'
set key right top

plot \
"< grep list-none-m lab2b_list.csv | grep -E \",([0-9]|12),1000,1,\"" using ($2):(1000000000/($6)) \
title 'list=1' with linespoints lc rgb 'red', \
"< grep list-none-m lab2b_list.csv | grep -E \",([0-9]|12),1000,4,\"" using ($2):(1000000000/($6)) \
title 'lists=4' with linespoints lc rgb 'green', \
"< grep list-none-m lab2b_list.csv | grep -E \",([0-9]|12),1000,8,\"" using ($2):(1000000000/($6)) \
title 'lists=8' with linespoints lc rgb 'blue', \
"< grep list-none-m lab2b_list.csv | grep -E \",([0-9]|12),1000,16,\"" using ($2):(1000000000/$6) \
title 'lists=16' with linespoints lc rgb 'orange'

set title "Throughput vs. Number of Threads for Spin-lock Sub-lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (op/s)"
set output 'lab2b_5.png'
set key right top

plot \
"< grep list-none-s lab2b_list.csv | grep -E \",([0-9]|12),1000,1,\"" using ($2):(1000000000/($6)) \
title 'lists=1' with linespoints lc rgb 'red', \
"< grep list-none-s lab2b_list.csv | grep -E \",([0-9]|12),1000,4,\"" using ($2):(1000000000/($6)) \
title 'lists=4' with linespoints lc rgb 'green', \
"< grep list-none-s lab2b_list.csv | grep -E \",([0-9]|12),1000,8,\"" using ($2):(1000000000/($6)) \
title 'lists=8' with linespoints lc rgb 'blue', \
"< grep list-none-s lab2b_list.csv | grep -E \",([0-9]|12),1000,16,\"" using ($2):(1000000000/($6)) \
title 'lists=16' with linespoints lc rgb 'orange'

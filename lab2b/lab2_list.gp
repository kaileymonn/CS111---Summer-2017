#! /usr/bin/gnuplot
#
# purpose:
#  generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv lab2_list2.csv
# 1. test name
# 2. # threads
# 3. # iterations per thread
# 4. # lists
# 5. # operations performed (threads x iterations x (ins + lookup + delete))
# 6. run time(ns)
# 7. average time per operation(ns)
# 8. average wait-for-lock time(ns)
#
# output:
#   lab2b_1.png ... throughput vs number of threads for mutex and spin­lock synchronized list operations.
#   lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex­synchronized list operations.
#   lab2b_3.png ... successful iterations vs threads for each synchronization method.
#   lab2b_4.png ... throughput vs number of threads for mutex synchronized partitioned lists.
#   lab2b_5.png ... throughput vs number of threads for spin­lock-synchronized partitioned lists.
#

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png 
set title "Throughput vs Threads for Mutex/Spin-lock List Operations"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput(op/s)"
set logscale y 10
set output 'lab2b_1.png'
set key left top

plot \
     "< grep list-none-m lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'mutex list, 1000 iterations' with linespoints lc rgb 'red', \
     "< grep list-none-s lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'spin-lock list, 1000 iterations' with linespoints lc rgb 'blue'


# lab2b_2.png
set title "Lock Wait-Time/Average Time-per-op vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Time(ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep list-none-m lab2b_list.csv | grep 1000,1," using ($2):($7) \
     title 'Average time per operation' with linespoints lc rgb 'red', \
     "< grep list-none-m lab2b_list.csv | grep 1000,1," using ($2):($8) \
     title 'Average lock wait-time' with linespoints lc rgb 'blue'



# lab2b_3.png 
set title "Number of Iterations vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep list-id-m lab2_list2.csv" using ($2):($3) \
     title 'Mutex' with points lc rgb 'red', \
     "< grep list-id-s lab2_list2.csv" using ($2):($3) \
     title 'Spin-lock' with points lc rgb 'green', \
     "< grep list-id-none lab2_list2.csv" using ($2):($3) \
     title 'No-sync' with points lc rgb 'blue'

# lab2_4.png
set title "Throughput vs Threads(Mutex locks)"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput(op/s)"
set output 'lab2b_4.png'
set key right top

plot \
     "< grep list-none-m lab2_list2.csv | grep 1000,1," using ($2):(1000000000/($7)) \
     title '# sublists = 1' with linespoints lc rgb 'red', \
     "< grep list-none-m lab2_list2.csv | grep 1000,4," using ($2):(1000000000/($7)) \
     title '# sublists = 4' with linespoints lc rgb 'green', \
     "< grep list-none-m lab2_list2.csv | grep 1000,8," using ($2):(1000000000/($7)) \
     title '# sublists = 8' with linespoints lc rgb 'blue', \
     "< grep list-none-m lab2_list2.csv | grep 1000,16," using ($2):(1000000000/$7) \
     title '# sublists = 16' with linespoints lc rgb 'orange'

# lab2_5.png
set title "Throughput vs Threads(Spin-locks)"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput(op/s)"
set output 'lab2b_5.png'
set key right top

plot \
     "< grep list-none-s lab2_list2.csv | grep 1000,1," using ($2):(1000000000/($7)) \
     title '# sublists = 1' with linespoints lc rgb 'red', \
     "< grep list-none-s lab2_list2.csv | grep 1000,4," using ($2):(1000000000/($7)) \
     title '# sublists = 4' with linespoints lc rgb 'green', \
     "< grep list-none-s lab2_list2.csv | grep 1000,8," using ($2):(1000000000/($7)) \
     title '# sublists = 8' with linespoints lc rgb 'blue', \
     "< grep list-none-s lab2_list2.csv | grep 1000,16," using ($2):(1000000000/($7)) \
     title '# sublists = 16' with linespoints lc rgb 'orange'
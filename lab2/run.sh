#! /bin/bash

# runs connect4 with depth from 1 to 10 with 1 to 8 processors and saves logs to
# time/time_PROC_DEPTH.txt file

EXE=./connect4

make clean && make

for depth in $(seq 10)
do
    for proc in $(seq 8) 
    do
        mpirun -n $proc $EXE $depth < moves.in 2> time/time_${proc}_${depth}.txt
    done
done

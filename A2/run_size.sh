#!/bin/bash
echo "Number of threads: $1 $2 $3 $4 $5 $6";

echo "RUNNING TEST CASES..."

echo "RUNNING OMP IMPLEMENTATION..."

for filename in sample_inputs/*.in; do
    echo "RUNNING $filename"
    for ((counter = 0; counter < 3; counter ++)) do
        echo "run $counter"
        ./goi_cuda "$filename" deathToll.out $1 $2 $3 $4 $5 $6
    done
done

echo "DONE"


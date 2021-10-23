#!/bin/bash
echo "Number of threads: $1 $2 $3 $4 $5 $6";

echo "RUNNING TEST CASES..."

echo "RUNNING OMP IMPLEMENTATION..."

for filename in testcases/*.in; do
    echo "RUNNING $filename"
    for ((counter = 0; counter < 1; counter ++)) do
        echo "run $counter"
        ./goi_cuda "$filename" deathToll.out $1 $2 $3 $4 $5 $6
    done
done

echo "DONE"


#!/bin/bash
echo "Number of threads: $1";

echo "RUNNING TEST CASES..."

echo "RUNNING OMP IMPLEMENTATION..."

cd ..
for filename in testcases/*.in; do
    echo "RUNNING $filename"
    perf stat -e cache-misses -- ./goi_omp "$filename" deathToll.out $1
done

for filename in testcases/*.in; do
    echo "RUNNING $filename"
    perf stat -e cache-misses -- ./goi_threads "$filename" deathToll.out $1
done

echo "DONE"


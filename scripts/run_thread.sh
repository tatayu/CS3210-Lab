#!/bin/bash
echo "Number of threads: $1";

echo "RUNNING TEST CASES..."

echo "RUNNING THREAD IMPLEMENTATION..."

cd ..
for filename in testcases/*.in; do
    echo "RUNNING $filename"
    for ((counter = 0; counter < 3; counter ++)) do
        echo "run $counter"
        ./goi_threads "$filename" deathToll.out $1
        var="$(cat deathToll.out)"
        echo "output value for $filename is $var"
    done
done

echo "DONE"


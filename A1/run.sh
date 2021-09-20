#!/bin/bash
echo "RUNNING TEST CASES..."

echo "RUNNING OMP IMPLEMENTATION..."

for filename in test_cases/*.in; do
    echo "RUNNING $filename"
    for(counter = 0; counter < 3; counter ++); do
        echo "run $counter"
        ./goi_omp.out test_cases/"$filename" deathToll.out
    done
done

echo "RUNNING THREAD IMPLEMENTATION..."

for filename in test_cases/*.in; do
    echo "RUNNING $filename"
    for(counter = 0; counter < 3; counter ++); do
        echo "run $counter"
        ./goi_threads.out test_cases/"$filename" deathToll.out
    done
done

echo "DONE"


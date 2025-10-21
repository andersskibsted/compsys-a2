#!/usr/bin/env sh

outputfile="fib-input-random.txt"
count=500

> "$outputfile"

for i in $(seq 1 $count); do
    echo $(( RANDOM % 10 + 1 )) >> fib-input-random.txt
done

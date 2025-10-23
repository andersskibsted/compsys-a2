#!/usr/bin/env sh

bytes_per_file=10000000  

mkdir -p "test"

for (( i=0; i<20; i++ )); do
    filename="test/histogram-input-$i.bin"
    head -c $bytes_per_file /dev/urandom > "$filename"
done

echo ""
echo "Testing 1 thread:"
time -p ./fhistogram-mt -n 1 "test"

echo ""
echo "Testing 2 thread:"
time -p ./fhistogram-mt -n 2 "test"

echo ""
echo "Testing 4 thread:"
time -p ./fhistogram-mt -n 4 "test"

echo ""
echo "Testing 10 threads:"
time -p ./fhistogram-mt -n 10 "test" 

echo ""
echo "Testing 20 threads:"
time -p ./fhistogram-mt -n 20 "test" 

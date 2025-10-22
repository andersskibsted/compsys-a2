#!/usr/bin/env sh

bytes_per_file=10000000  

mkdir -p "test"

for (( i=0; i<20; i++ )); do
    filename="test/histogram-input-$i.bin"
    head -c $bytes_per_file /dev/urandom > "$filename"
done

time -p ./fhistogram-mt -n 1 "test"

echo ""
echo "Testing many threads:"
time -p ./fhistogram-mt -n 1000 "test" 
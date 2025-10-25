#!/bin/bash

search_term=are

#!/usr/bin/env sh

make 
mkdir -p "test-fauxgrep"

for i in {1..10}; do
    filename="test-fauxgrep/fauxgrep-input-$i.txt"
    head -c 10000000 /dev/urandom | base64 | fold -w 80 > "$filename"
done 


# Test correctnes with fauxgrep

./fauxgrep-mt -n 5 in test-fauxgrep > fil1.txt
./fauxgrep in test-fauxgrep > fil2.txt

diff <(sort fil1.txt) <(sort fil2.txt) > differences.txt
equal=$?
echo "Testing correctnes:"
echo ""

if [ $equal -eq 0 ]; then
  echo "fauxgrep-mt and fauxgrep produces identical output."
else
    
  echo "fauxgrep-mt and fauxgrep produces identical output."
  cat differences.txt
fi
rm differences.txt
rm fil1.txt
rm fil2.txt

echo ""
echo ""
# Test throughput
echo "Testing throughput and speed"
echo ""

echo "Testing 1 thread"
time -p ./fauxgrep-mt -n 1 ${search_term} test-fauxgrep > /dev/null
echo ""
echo "Testing 2 threads"
time -p ./fauxgrep-mt -n 2 ${search_term} test-fauxgrep > /dev/null

echo ""
echo "Testing 4 threads"
time -p ./fauxgrep-mt -n 4 ${search_term} test-fauxgrep > /dev/null

echo ""
echo "Testing 10 threads"
time -p ./fauxgrep-mt -n 10 ${search_term} test-fauxgrep > /dev/null

echo ""
echo "Testing 20 threads"
time -p ./fauxgrep-mt -n 20 ${search_term} test-fauxgrep > /dev/null

echo ""
echo "Testing unthread version"
time -p ./fauxgrep ${search_term} test-fauxgrep > /dev/null

rm test-fauxgrep/*
rmdir test-fauxgrep

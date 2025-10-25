#!/bin/bash

search_term=are

#!/usr/bin/env sh

mkdir -p "test-fauxgrep"

for i in {1..10}; do
    filename="test-fauxgrep/fauxgrep-input-$i.txt"
    head -c 10000000 /dev/urandom | base64 | fold -w 80 > "$filename"
    #head -c $bytes_per_file /dev/urandom > "$filename"
    #cat /dev/urandom | base64 | head -c 8000 | fold -w 80
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
echo ""
echo ""
# Test throughput
echo "Testing throughput"
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


# Test speed 
echo ""
echo "Speed tests:"
echo ""

echo "fauxgrep-mt with 5 threads small files"
time ./fauxgrep-mt -n 5 for test-fauxgrep > /dev/null
   
echo ""

echo "fauxgrep-mt with 5 threads small files"
time ./fauxgrep-mt -n 5 for test-fauxgrep > /dev/null

echo "fauxgrep with small files"
time ./fauxgrep for test-fauxgrep > /dev/null

echo ""

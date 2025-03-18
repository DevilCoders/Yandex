#!/bin/bash

if [ -e WORK ]; then
   rm -rf WORK;
fi;
mkdir WORK;
./sanitizer -t q.tmp -o WORK -c windows-1251 -n 40 -b "http://www.yahoo.com" test/in/*.h*;
if [ -e q.tmp ]; then
    rm q.tmp;
fi;

qDiff="N";

pushd WORK;

for f in *.h*;
do
  cmp $f ../test/out/$f > /dev/null
  if [ $? -ne 0 ]; then
    qDiff="Y";
    echo "See differences in test/out/$f WORK/$f";
  else
    rm ../WORK/$f;
  fi;
done;
popd;

if [ "$qDiff" == "N" ]; then 
   rm -rf WORK;
   echo "Test: OK";
else
   echo "Test FAILED";
fi;


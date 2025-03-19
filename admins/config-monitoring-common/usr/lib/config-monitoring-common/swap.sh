#!/bin/bash
#
# how many percent of swap is used

fullswap=`awk 'NR == 2 {print $3}' /proc/swaps`
usedswap=`awk 'NR == 2 {print $4}' /proc/swaps`

exec 2> /dev/null;

if [ -z $fullswap ] || [ -z $usedswap ]; then
  echo "0;swap is not used";
else
  using=$(($usedswap*100/$fullswap))
  if [[ $using -ge 30 ]]; then
    echo "1;swap is used for more than 30%!";
  else
    echo "0;OK";
  fi
fi
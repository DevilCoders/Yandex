#!/bin/bash

>/dev/null
2>/dev/null

# check that host is not under maintenance
iptruler all dump | grep -q "iptruler all down" && exit 0

# check that host have run cocaine-http-proxy and nginx proccess
ubic status cocaine-http-proxy | grep -q running || exit 0
service nginx status | grep -q running || exit 0

# fix if problem exist
PID=$(pgrep cocaine-http-p)
for CPID in $PID; do
  PLIMIT=$(grep "Max open files" /proc/$CPID/limits | awk '{print $4}')
  COEF=0.7
  CSIZE=$(echo "scale=0; $PLIMIT * $COEF / 1" | bc -l)
  FDC=$(ls -1 /proc/$CPID/fd/ | wc -l)
  if (( $FDC > $CSIZE )); then
    iptruler all down
    sleep 60
    ubic restart cocaine-http-proxy
    ubic restart cocaine-runtime
    service nginx restart
    sleep 30
  else
    exit 0
  fi
  while :; do
    if (ubic status cocaine-http-proxy | grep -q running) && (ubic status cocaine-runtime | grep -q running); then
      iptruler all up
      exit 0
    fi
    sleep 30;
  done
done

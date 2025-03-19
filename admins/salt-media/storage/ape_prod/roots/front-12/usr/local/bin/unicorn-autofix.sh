#!/bin/bash

>/dev/null
2>/dev/null

# check that host is not under maintenance
iptruler all dump | grep -q "iptruler all down" && exit 0

# check that host have run cocaine-http-proxy and nginx proccess
ubic status cocaine-http-proxy | grep -q running || exit 0
service nginx status | grep -q running || exit 0

# fix if problem exist
if ! (timeout -s SIGKILL 60 monrun -r cocaine-unicorn | grep -i ok); then
  iptruler all down
  sleep 60
  ubic restart cocaine-runtime
  sleep 30
fi
while :; do
  if (ubic status cocaine-runtime | grep -q running); then
    iptruler all up
    exit 0
  fi
  sleep 30;
done


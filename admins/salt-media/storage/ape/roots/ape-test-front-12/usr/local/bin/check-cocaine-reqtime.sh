#!/bin/bash

for i in 1 2 3 4 5; do
  curl -s -H 'Host:echo' 'http://localhost:82/' >/dev/null 2>&1
  sleep $i
done

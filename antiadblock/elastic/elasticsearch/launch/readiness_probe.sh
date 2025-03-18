#!/bin/sh

result=$(curl -o /dev/null -w "%{http_code}" --max-time 60 -XGET -s localhost:8890)
rc=$?
if [[ ${rc} -ne 0 ]]; then
  exit 1
fi

if [[ ${result} == "200" ]]; then
  exit 0
fi

exit 2

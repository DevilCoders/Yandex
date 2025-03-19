#!/bin/bash

FILE="/var/log/s3/goose-maintain/check_report_alert.log"

if [ -f $FILE ]; then
  SIZE=$(stat -c %s $FILE)
  if [[ $SIZE != 0 ]]; then
    echo "2; CRIT, $FILE not empty"
    exit 0
  else
    echo "0; OK"
    exit 0
  fi
else
  echo "1; WARN, $FILE not found"
  exit 0
fi


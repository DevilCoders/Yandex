#!/bin/bash

FILE="/var/log/s3/billing/bad_values.log"
BADSIZE=51200

if [ -f $FILE ]; then
  SIZE=$(stat -c %s $FILE)
  if [[ $SIZE != 0 ]]; then
    if [[ $1 == "bad" ]]; then
      if (( $SIZE > $BADSIZE )); then
        echo "2; CRIT, $FILE too big, $SIZE bytes"
        exit 0
      else
        echo "0; OK, size $SIZE bytes"
        exit 0
      fi
    else
      echo "2; CRIT, $FILE not empty"
      exit 0
    fi
  else
    echo "0; OK"
    exit 0
  fi
else
  echo "1; WARN, $FILE not found"
  exit 0
fi


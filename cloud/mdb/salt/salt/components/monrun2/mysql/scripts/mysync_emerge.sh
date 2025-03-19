#!/bin/bash
  
die () {
    echo "$1;$2"
    exit 0
}


if [ -f "/var/run/mysync/mysync.emerge" ]; then
        die 2 "$(head -n 1 /var/run/mysync/mysync.emerge)"
else
        die 0 "OK"
fi

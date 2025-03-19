#!/bin/bash

set -e
. utils.sh

if [ $# -ne 2 ]; then
  echo "Usage: sr-http.sh FAMILY TARGET_IDX"
  exit 2
fi

MAXTIME=10
FAMILY=$1
TARGET_IDX=$2
TARGET_ADDR=$(get_metadata_value attributes/rb-targets | jq -r ".$TARGET_IDX.$FAMILY")

if curl -m $MAXTIME -v http://$TARGET_ADDR/ > /dev/null; then
  pass "connection succeded"
else
  fail "connection failed to target $TARGET_ADDR"
fi

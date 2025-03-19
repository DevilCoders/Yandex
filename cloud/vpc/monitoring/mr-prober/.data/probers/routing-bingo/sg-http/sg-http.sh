#!/bin/bash

set -e
. utils.sh

if [ $# -ne 2 ]; then
  echo "Usage: sg-http.sh FAMILY TARGET_NAME"
  exit 2
fi

MAXTIME=10
FAMILY=$1
TARGET_NAME=$2
TARGET_HOST=${TARGET_NAME}.${DOMAINNAME}

case "$FAMILY" in
  inet) CURLOPT=-4 ;;
  inet6) CURLOPT=-6 ;;
  *) error "invalid family $FAMILY" ;;
esac

EXPECT=$(get_metadata_value attributes/prober-expect)
echo "Testing connection to $TARGET_HOST (expected: $EXPECT)"
case "$EXPECT" in
  allow)
    if curl -m $MAXTIME $CURLOPT http://$TARGET_HOST/ > /dev/null; then
      pass "connection succeded"
    else
      fail "connection failed despite allow on SG"
    fi
  ;;
  deny)
    if curl --connect-timeout 4 -m $MAXTIME $CURLOPT http://$TARGET_HOST/ > /dev/null; then
      fail "connection successful despite deny on SG"
    else
      pass "connection failed"
    fi
  ;;
  *)
    error "prober_expect should be set either to 'allow' or to 'deny'"
  ;;
esac

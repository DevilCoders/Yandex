#!/bin/bash

set -e
. utils.sh

if [ $# -ne 1 ]; then
  echo "Usage: sr-nat-route.sh TARGET_IDX"
  exit 2
fi

TARGET_IDX=$1
TARGET_ADDR=$(get_metadata_value attributes/rb-targets | jq -r ".[$TARGET_IDX]")
EXPECTED_NEXTHOP=$(get_metadata_value attributes/expected-nexthops | jq -r ".[$TARGET_IDX]")

OUTPUT=$(mktemp -t sr-nat-route-XXXXXX)
trap "rm $OUTPUT" EXIT

if ! traceroute $TARGET_ADDR | tee $OUTPUT ; then
  error "traceroute to target $TARGET_ADDR has failed"
fi

if grep -q "nat-instance-$EXPECTED_NEXTHOP" $OUTPUT; then
  pass "correct NAT instance ($EXPECTED_NEXTHOP) was picked by static route"
else
  fail "expected NAT instance ($EXPECTED_NEXTHOP) is not found in route trace"
fi

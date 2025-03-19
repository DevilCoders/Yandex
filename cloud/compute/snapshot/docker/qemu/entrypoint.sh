#!/bin/bash
set -eu

socat -s tcp-listen:1234,max-children=100,fork,cool-write unix-connect:/connect/image-proxy,cool-write &
sleep 1

CMD=""
for PARAM in "$@"; do
    CMD="$CMD\"$PARAM\" "
done
exec bash -c "$CMD"

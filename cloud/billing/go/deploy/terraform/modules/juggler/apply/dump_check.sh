#!/bin/bash

set -e

cd `dirname $0`

t=$(mktemp ./checkXXXXX) || exit
trap "rm -f -- '$t'" EXIT

cat >> $t

FILE=`cat $t|jq -r .mark`

mkdir -p ./.checks

cat $t | jq -r .checks > ./.checks/$FILE

echo '{}'

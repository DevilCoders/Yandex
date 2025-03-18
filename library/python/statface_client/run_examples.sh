#!/usr/bin/env bash

set -x

AUTHPATH="$HOME/.statbox"
AUTHCONFIG="$AUTHPATH/statface_auth.yaml"

mkdir -p $AUTHPATH

if [ ! -e "$AUTHCONFIG" ]
then
    cp nothing_interesting $AUTHCONFIG
fi

example_scripts=`find examples -iname '*.py'`
for e in $example_scripts
do
    python $e >/dev/null
done

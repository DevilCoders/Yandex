#!/bin/sh

if [ -z "$1" ]; then
	echo "No params for $0"
	exit 1
fi

cat "$1" | awk -F "\t" '{print $1"\t"$2"\t"$6"\t"length($6);}'

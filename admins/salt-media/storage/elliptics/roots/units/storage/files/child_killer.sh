#!/bin/bash

usage() {
cat<<HELP
SYNOPSIS
  $0  [ OPTIONS ]
OPTIONS
  -k,
      Kill. Default: false
  -h,
      Print a help message.

HELP
    exit 0
}

KILL=false

while getopts "hk" opt; do
    case $opt in
        k)
            KILL=true
            ;;
        h)
            usage && exit 1
            ;;
        ?)
            usage && exit 1
            ;;
    esac
done

cmd_pattern="dnet_recovery"

proc=(`ps xao pid,ppid,comm | grep -P "$cmd_pattern" | awk '{if ($2 == 1) print $1}'`)

if [ ${#proc[@]} -gt 0 ]
then
	echo "2; ${#proc[@]} processes don't have a parent"

	if [ $KILL == true ]
	then
		echo "Kill ${proc[@]}"
		echo "${proc[@]}" | xargs kill
	fi
else
	echo "0; Ok"
fi

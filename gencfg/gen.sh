#!/usr/bin/env bash

run_checks=$1

if [ "$run_checks" != "" ] && [ "$run_checks" != "run_checks" ] && [ "$run_checks" != "precommit" ] ; then
    echo "Usage: ./gen.sh [run_checks]"
    exit 1
fi

source `dirname "${BASH_SOURCE[0]}"`/scripts/run.sh
run make clean

if [ "$run_checks" == "" ]; then
    run /usr/bin/time -f "[TIME]: %C %E" make -j10
elif [ "$run_checks" == "precommit" ]; then
    run /usr/bin/time -f "[TIME]: %C %E" make -j10 build/precommit
else
    run /usr/bin/time -f "[TIME]: %C %E" make -j10 build/check
fi

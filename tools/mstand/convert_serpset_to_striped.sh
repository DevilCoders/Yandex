#!/bin/bash

set -e
set -o pipefail

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Wrong params. Syntax: $0 <raw-serpset> <jsonlines-serpset.jsonl> [--convert-only] [--sort-keys]"
    exit 1
fi

src="$1"
shift

dst="$1"
shift

jq_bin=jq
if ! which jq > /dev/null; then
    # echo "System jq not found, trying to use local"
    jq_bin='./jq'
    if ! [ -x "${jq_bin}" ]; then
        echo "No jq utility found anywhere."
        exit 2
    fi
fi

read_cmd="gunzip --stdout"
if [ "$1" == "--convert-only" ]; then
	read_cmd="cat"
	shift
fi

sort_flag=""
if [ "$1" == "--sort-keys" ]; then
	sort_flag="-S"
	shift;
fi

${read_cmd} "${src}" | "${jq_bin}" -c -M ${sort_flag} '.[]' > "${dst}"

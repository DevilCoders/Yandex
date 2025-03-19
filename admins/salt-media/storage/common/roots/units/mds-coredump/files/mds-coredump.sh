#!/bin/bash

usage() {
cat<<HELP
SYNOPSIS
  $0  [ OPTIONS ]
OPTIONS
  -c,
      Core limit. Default: 0
  -i,
      Ignore pattern. Default: none
  -h,
      Print a help message.

HELP
    exit 0
}

while getopts "hl:i:" opt; do
    case "${opt}" in
        l)
            LIMIT="${OPTARG}"
            ;;
        i)
            IGNORE="${OPTARG}"
            ;;
        h)
            usage && exit 1
            ;;
        ?)
            usage && exit 1
            ;;
    esac
done

min_coredumps="${LIMIT:-0}"
ignore=${IGNORE:-"none"}

core_prefix=$(sysctl -a 2>/dev/null | grep kernel.core_pattern | awk '{print $NF}' | sed 's/\..*//g')

cores_list_file=$(mktemp)
ls -1 "${core_prefix}"* 2>/dev/null > "${cores_list_file}"

ignore_pattern="${core_prefix}\.(${ignore})"

count_cores=$(grep -vPc "${ignore_pattern}" "${cores_list_file}")
count_ignored=$(grep -Pc "${ignore_pattern}" "${cores_list_file}")

if [ "${count_cores}" -gt "${min_coredumps}" ]
then
    echo "2; ${count_cores} coredumps found and ${count_ignored} coredumps ignored"
else
    echo "0; ${count_ignored} coredumps ignored"
fi

rm "${cores_list_file}"

#!/bin/bash
#set -x #echo on

if [[ -z "$@" ]]; then
    if [[ "${COMMAND_DEFAULT+x}" ]]; then
        echo "${COMMAND_DEFAULT}"
    elif [[ ! -z "${COMMAND_DEFAULT_TAIL_N}" ]]; then
        echo "tail -n ${COMMAND_DEFAULT_TAIL_N}"
    else
        echo "tail -n 1"
    fi
else
    echo "$@"
fi

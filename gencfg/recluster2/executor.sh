#!/usr/bin/env bash

function select_action()
{
    if [[ ${#BASH_ARGV[@]} -ne 1 ]]; then
        echo "Usage: $0 start|mid|end|all"
        exit 1
    fi

    start_func=$1
    mid_func=$2
    end_func=$3
    case "$BASH_ARGV" in
        start)
            ${start_func}
            ;;
        mid)
            ${mid_func}
            ;;
        end)
            ${end_func}
            ;;
        all)
            ${start_func}
            ${mid_func}
            ${end_func}
            ;;
        *)
            echo "Usage: $0 start|mid|end|all"
            exit 1
    esac
}

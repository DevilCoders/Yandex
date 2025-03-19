#!/bin/bash
#set -x #echo on
IOT_ERROR="grep \"\\\"level\\\":\\\"ERROR\\\"\""

IOT_JQ_SORT_BY_TIMESTAMP="grep \"^{\" | jq --sort-keys -s \". | sort_by(.ts)\""

function iot_jq_sort_by_timestamp() {
    if [[ "$JQ_DISABLE" == "true" ]]; then
        echo "cat"
    elif [[ ! -z "$JQ_TSV" ]]; then
        echo "grep \"^{.*}$\" | jq -r \"[${JQ_TSV}] | @tsv\""
    elif [[ ! -z "$JQ_MAP" ]]; then
        if [[ "$JQ_MAP_DISABLE_SORT" == "true" ]]; then
            echo "grep \"^{.*}$\" | jq -s \"map(${JQ_MAP})\" "
        else
            echo "grep \"^{.*}$\" | jq -s \"map(${JQ_MAP}) | sort_by(.ts)\" "
        fi
    else
        echo ${IOT_JQ_SORT_BY_TIMESTAMP}
    fi
}


# examples:
#  HOST_FILTER=vla$
#  HOST_FILTER='(vla|sas)$'
#  HOST_FILTER='(?<!vla)$'
#  HOST_FILTER='(?<!vla|sas)$'
HOST_FILTER_CMD='BEGIN{$f=shift @ARGV; $_=`hostname`; chomp; $x=!$f || !$_ || m/$f/; print $f } print if $x'
if [ -n "$HOST_FILTER" ]; then
    NAME_FILTER_CMD="perl -lne'$HOST_FILTER_CMD' '$HOST_FILTER' |"
fi

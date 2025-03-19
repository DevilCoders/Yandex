#!/bin/bash
#set -x #echo on

HOSTNAME_TO_LOG="sed \"s/}$/,\\\"hostname\\\":\\\"\$(hostname)\\\"}/\""

JQ_SORT_BY_TIMESTAMP="grep \"^{\" | jq --sort-keys -s \". | sort_by(.timestamp)\""

# https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
GRPC_ERROR="egrep -v \"\\\"grpc_status_code\\\":(0|1|3|5|6|7|8|9|10|11|16)[^0-9]\""
GRPC_OK="egrep \"\\\"grpc_status_code\\\":(0)[^0-9]\""

HTTP_ERROR="grep \"\\\"status\\\":5[0-9][0-9][^0-9]\""
HTTP_4XX="grep \"\\\"status\\\":4[0-9][0-9][^0-9]\""

function jq_sort_by_timestamp() {
    if [[ "$JQ_DISABLE" == "true" ]]; then
        echo "cat"
    elif [[ ! -z "$JQ_TSV" ]]; then
        echo "grep \"^{.*}$\" | jq -r \"[${JQ_TSV}] | @tsv\""
    elif [[ ! -z "$JQ_MAP" ]]; then
        if [[ "$JQ_MAP_DISABLE_SORT" == "true" ]]; then
            echo "grep \"^{.*}$\" | jq -s \"map(${JQ_MAP})\" "
        else
            echo "grep \"^{.*}$\" | jq -s \"map(${JQ_MAP}) | sort_by(.timestamp)\" "
        fi
    else
        echo ${JQ_SORT_BY_TIMESTAMP}
    fi
}

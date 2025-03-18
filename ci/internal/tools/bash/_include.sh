#!/bin/bash

set -e

STAGE=$1
shift

if [[ -z "$STAGE" ]]; then
    echo "Expect tool <stage-name>"
    exit 1
fi

. $(dirname "$0")/_stages.sh

function run() {
    STAGE="$1"
    shift

    . $(dirname "$0")/_stages.sh

    if [[ -z "$UNIT" ]]; then
        echo "Unable to find stage $STAGE"
        exit 1
    fi

    echo ""
    echo "Stage: $STAGE"
    for HOST in $(ya tool dctl list endpoint "$STAGE"."$UNIT" | grep 'yp-c.yandex.net' | awk -F '|' '{print $4}'); do
        main_func "$BOX.$HOST" "$*"
    done
}

if [[ "$STAGE" == "all" ]]; then
    for STAGE in ${ALL_STAGES[@]}; do
        run "$STAGE" "$*"
    done
else
    run "$STAGE" "$*"
fi

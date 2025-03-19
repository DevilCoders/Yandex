#!/usr/bin/env bash

source "${BASH_SOURCE%/*}/common.sh"

TARGET=`get_instance_field $@ "target"`
EC="$?"
if [[ "$EC" -ne "0" ]]; then
    exit "$EC"
fi
TARGET=`echo "$TARGET" | sed -e's/^/ --target /'`

if [[ -z "$TARGET" ]]; then
    # last line of defence: apply should never be launched w/o explicit targets
    echo "Intended to run apply without targets, exiting"
    exit 1
fi

log_and_eval "terraform apply \
        ${TARGET} \
        --auto-approve \
        --var yc_token=${YC_OAUTH} \
        --var yandex_token=${YT_OAUTH}"

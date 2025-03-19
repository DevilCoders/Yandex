#!/usr/bin/env bash

source "${BASH_SOURCE%/*}/common.sh"

TARGET=`get_instance_field $@ "target"`
EC="$?"
if [[ "$EC" -ne "0" ]]; then
    exit "$EC"
fi
TARGET=`echo "$TARGET" | sed -e's/^/ --target /'`

log_and_eval "terraform plan \
        ${TARGET} \
        --var yc_token=${YC_OAUTH} \
        --var yandex_token=${YT_OAUTH}"
exit "$?"

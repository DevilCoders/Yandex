#!/bin/bash

set -e

cd `dirname $0`

export JUGGLER_OAUTH_TOKEN="${TF_VAR_yt_token}"

BIN_DIR='../../.bin'
BIN=`which juggler_cli || echo ${BIN_DIR}/juggler_cli/juggler_cli`

if [[ ! -e  $BIN ]]; then
    echo juggler_cli not found
    exit 99
fi

HOST=$1
DESCRIPTION=$2

echo downtime --api http://juggler-api.search.yandex.net --host $HOST --description $DESCRIPTION

$BIN downtime --api http://juggler-api.search.yandex.net --host $HOST --description "$DESCRIPTION"

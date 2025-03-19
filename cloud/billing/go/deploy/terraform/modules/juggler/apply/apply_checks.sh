#!/bin/bash

set -e

cd `dirname $0`

BIN_DIR='../../.bin'
BIN=`which juggler_cli || echo ${BIN_DIR}/juggler_cli/juggler_cli`

if [[ ! -e  $BIN ]]; then
    echo juggler_cli not found
    exit 99
fi

MARK=$1

$BIN load --api http://juggler-api.search.yandex.net -m $MARK -s ./.checks/$MARK  -v

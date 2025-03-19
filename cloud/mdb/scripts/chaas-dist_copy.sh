#!/usr/bin/env bash

set -e
[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

if [[ $# -ne 1 ]]; then
    echo "Usage: `basename $0` <version>" >&2
    exit 1
fi

version="$1"

dbaas dist move -r yandex-trusty clickhouse-server $version
dbaas dist copy --from yandex-trusty clickhouse-server $version mdb-bionic

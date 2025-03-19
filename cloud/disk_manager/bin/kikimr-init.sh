#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
DATA_DIR="$WORKING_DIR/data"
CONFIGS=" \
    init_storage \
    init_compute \
    init_root_storage \
    init_databases \
    init_cms \
    "

set -e

for config in $CONFIGS; do
    echo "Running $config"
    bash "$DATA_DIR/dynamic/$config.bash"
done

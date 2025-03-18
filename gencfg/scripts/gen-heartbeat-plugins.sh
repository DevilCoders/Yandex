#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

GENERATED_DIR="./generated/hplugins"

mkdir -p "${GENERATED_DIR}"

MYARGS=()

for project in instanceusage instanceusagev2 hwdata unworking; do
#    MYARGS+=("./heartbeat/generate_plugin.py -p ${project} -o ${GENERATED_DIR}/${project}.py && pyflakes ${GENERATED_DIR}/${project}.py")
    MYARGS+=("./custom_generators/heartbeat/generate_plugin.py -p ${project} -o ${GENERATED_DIR}/${project}.py") # temporary disabled pyflakes
done

par "${MYARGS[@]}"

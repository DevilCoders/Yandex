#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 (curdb|api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ${ROOT}/run.sh

TRANSPORT=$1
GENPY="$CUSTOM_PYTHON ${ROOT}/utils/generate_balancer_config.py"
PROJECTS_DIR="${ROOT}/configs/load_testing"

if [ "${TRANSPORT}" == "curdb" ]; then
    GENERATED_DIR="${ROOT}/../../generated/balancer/load_testing"
elif [ "${TRANSPORT}" == "api" ]; then
    GENERATED_DIR="${ROOT}/generated/load_testing"
else
    echo "Unknown transport ${TRANSPORT}"
fi

mkdir -p "$GENERATED_DIR"

MYARGS=()

for project in `ls ${PROJECTS_DIR} | grep 'py$' | sed 's/\.py$//'`; do
    if [ "$project" == "__init__" ]; then
        continue
    fi

    MYARGS+=("${GENPY} -t ${TRANSPORT} -b load_testing -m ${project} -o ${GENERATED_DIR}/${project%.py}.cfg")
done

par "${MYARGS[@]}"

#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 (curdb|api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${ROOT}"/run.sh

TRANSPORT=$1
GENPY="$CUSTOM_PYTHON ${ROOT}/utils/generate_balancer_config.py"

if [ "${TRANSPORT}" == "curdb" ]; then
    GENERATED_DIR="${ROOT}/../../generated/balancer"
elif [ "${TRANSPORT}" == "api" ]; then
    GENERATED_DIR="${ROOT}/generated"
else
    echo "Unknown transport ${TRANSPORT}"
fi

mkdir -p "$GENERATED_DIR"

MYARGS=()

MYARGS+=("${GENPY} -b gencfg -o ${GENERATED_DIR}/gencfg.yandex-team.ru_production.cfg -t ${TRANSPORT}")


par "${MYARGS[@]}"

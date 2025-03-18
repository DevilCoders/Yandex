#!/usr/bin/env bash

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 (curdb|api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ${ROOT}/run.sh

TRANSPORT=$1
TOPOLOGY=$2
GENPY="$CUSTOM_PYTHON ${ROOT}/configs/improxy/improxy.py"
GENPY_ITDITP="$CUSTOM_PYTHON ${ROOT}/configs/improxy/imgthlb_recommendations.py"

if [ "${TRANSPORT}" == "curdb" ]; then
    GENERATED_DIR="${ROOT}/../../generated/improxy"
elif [ "${TRANSPORT}" == "api" ]; then
    GENERATED_DIR="${ROOT}/generated/improxy"
    if [ -z "${TOPOLOGY}" ]; then
        TOPOLOGY_ARG=""
    else
        TOPOLOGY_ARG="--topology $TOPOLOGY"
    fi
else
    echo "Unknown transport ${TRANSPORT}"
fi

mkdir -p "$GENERATED_DIR"


MYARGS=()

for subproject in images_sas images_man images_vla; do
    MYARGS+=("${GENPY} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} $GENERATED_DIR/improxy_${subproject}.cfg");
done

for subproject in itditp_sas itditp_man itditp_vla; do
    MYARGS+=("${GENPY_ITDITP} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} $GENERATED_DIR/improxy_${subproject}.cfg");
done

# internal cfgs
#for subproject in images_sas images_man; do
#    MYARGS+=("${GENPY} -m -t ${TRANSPORT} ${subproject} $GENERATED_DIR/improxy_${subproject}_internal.cfg");
#done

par "${MYARGS[@]}"

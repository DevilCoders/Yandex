#!/usr/bin/env bash

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 (curdb|api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ${ROOT}/run.sh

TRANSPORT=$1
TOPOLOGY=$2
GENPY="$CUSTOM_PYTHON ${ROOT}/configs/media-balancers/media-balancers.py"

if [ "${TRANSPORT}" == "curdb" ]; then
    GENERATED_DIR="${ROOT}/../../generated"
elif [ "${TRANSPORT}" == "api" ]; then
    GENERATED_DIR="${ROOT}/generated"
    if [ -z "${TOPOLOGY}" ]; then
        TOPOLOGY_ARG=""
    else
        TOPOLOGY_ARG="--topology $TOPOLOGY"
    fi
else
    echo "Unknown transport ${TRANSPORT}"
fi

MYARGS=()

#for subproject in improxy_priemka; do
#    PRJ_GENERATED_DIR="${GENERATED_DIR}/improxy"
#    mkdir -p "${PRJ_GENERATED_DIR}"
#    MYARGS+=("${GENPY} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} ${PRJ_GENERATED_DIR}/improxy_images_priemka.cfg");
#done

#for subproject in improxy-prestable_sas improxy-prestable_vla improxy-prestable_man; do
#    PRJ_GENERATED_DIR="${GENERATED_DIR}/improxy"
#    mkdir -p "${PRJ_GENERATED_DIR}"
#    MYARGS+=("${GENPY} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} ${PRJ_GENERATED_DIR}/${subproject}.cfg");
#done

#for subproject in cbrd_sas cbrd_man cbrd_vla; do
#    PRJ_GENERATED_DIR="${GENERATED_DIR}/improxy"
#    mkdir -p "${PRJ_GENERATED_DIR}"
#    LOCATION=$(echo $subproject | cut -d'_' -f2)
#    MYARGS+=("${GENPY} -t ${TRANSPORT} ${subproject} ${PRJ_GENERATED_DIR}/improxy_imgcbir_${LOCATION}.cfg");
#done

for subproject in rim_sas rim_man rim_vla rim_priemka; do
    PRJ_GENERATED_DIR="${GENERATED_DIR}/imgs-rim"
    mkdir -p "${PRJ_GENERATED_DIR}"
    MYARGS+=("${GENPY} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} ${PRJ_GENERATED_DIR}/imgs_${subproject}.cfg");
done

# for subproject in commercial_vla commercial_sas commercial_man commercial-hamster_sas commercial-hamster_man; do
#     PRJ_GENERATED_DIR="${GENERATED_DIR}/imgs-commercial"
#     mkdir -p "${PRJ_GENERATED_DIR}"
#     MYARGS+=("${GENPY} ${TOPOLOGY_ARG} -t ${TRANSPORT} ${subproject} ${PRJ_GENERATED_DIR}/imgs_${subproject}.cfg");
# done

par "${MYARGS[@]}"

#!/usr/bin/env bash

# error tolerance is an root of evil
set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 (curdb|api|cached-api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ${ROOT}/run.sh

TRANSPORT=$1
GENPY="$CUSTOM_PYTHON ${ROOT}/utils/generate_balancer_config.py"
if [ "${CUSTOM_GENPY}" != "" ]; then
    GENPY=$CUSTOM_GENPY
fi

if [ "${TRANSPORT}" == "curdb" ]; then
    GENERATED_DIR="${ROOT}/../../generated/l7-balancer"
elif [ "${TRANSPORT}" == "api" ]; then
    GENERATED_DIR="${ROOT}/generated/l7-balancer"
elif [ "${TRANSPORT}" == "cached-api" ]; then
    GENERATED_DIR="${ROOT}/generated/l7-balancer"
else
    echo "Unknown transport ${TRANSPORT}"
fi

mkdir -p "$GENERATED_DIR"

MYARGS=()

parallel_build="yes"  # default should be fast
generic_options="${GENPY} -t ${TRANSPORT} ${GENPY_VERBOSE} ${GENPY_ENABLE_CACHE} ${DISABLE_TRUNK_FALLBACK}"
main_options="${generic_options} --balancer l7heavy"
cache_options="--save-cache ${GENERATED_DIR}/gencfg_cache.pickle --print-stats"
if [ -n "${BUILD_GROUPS_CACHE}" ]; then
    main_options="$main_options --groups-cache ${GENERATED_DIR}/groups_cache.pickle $cache_options"
    generic_options="$generic_options $cache_options"
    # to avoid options duplication, clear them
    cache_options=""
    # groups cache should be built in single threaded mode
    parallel_build=""
fi

# cache heating
for l7heavy_subproject in \
    l7heavy_production_tun_sas \
    l7heavy_production_tun_man \
    l7heavy_production_tun_vla; do
    MYARGS+=("${main_options} -p ${l7heavy_subproject} -o ${GENERATED_DIR}/${l7heavy_subproject}.cfg $cache_options")
done

for cmd in "${MYARGS[@]}"; do
    ${cmd}
done

MYARGS=()
for l7heavy_subproject in \
    l7heavy_production_yaru_man \
    l7heavy_service_morda_man \
    l7heavy_service_morda_sas \
    l7heavy_service_morda_vla \
    l7heavy_service_search_man \
    l7heavy_service_search_sas \
    l7heavy_service_search_vla \
    l7heavy_production_yaru_sas \
    l7heavy_production_yaru_vla \
    l7heavy_production_yaru_testing_sas \
    l7heavy_production_tun_sas_only \
    l7heavy_production_tun_man_only \
    l7heavy_production_tun_vla_only \
    l7heavy_testing_tun_sas \
    l7heavy_testing_tun_man \
    l7heavy_testing_tun_vla \
    l7heavy_experiments_vla \
    l7heavy_production_fukraine_sas \
    l7heavy_production_fukraine_man \
    l7heavy_production_fukraine_vla \
        ; do
    MYARGS+=("${main_options} -p ${l7heavy_subproject} -o ${GENERATED_DIR}/${l7heavy_subproject}.cfg")
done


for any_subproject in man sas vla; do
    MYARGS+=("${generic_options} --balancer any -p ${any_subproject} -o ${GENERATED_DIR}/any_${any_subproject}.cfg")
done

if [ -n "$parallel_build" ] ; then
    par "${MYARGS[@]}"
else
    for cmd in "${MYARGS[@]}"; do
        ${cmd}
    done
fi

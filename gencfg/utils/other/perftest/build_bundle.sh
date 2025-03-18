#!/usr/bin/env bash

set -e


function colorecho() {
    CLR=$1
    shift

    if [ -t 1 ]; then
        ncolors=$(tput colors)
        if test -n "$ncolors" && test ${ncolors} -ge 8; then
            echo "$(tput setaf ${CLR})$(tput bold)$@$(tput sgr 0)"
        fi
    else
        echo "$@"
    fi
}


function wrap_run {
    if $@ 1>${ROOT_DIR}/last_command.out 2>${ROOT_DIR}/last_command.err; then
        :
    else
        colorecho 1 "Command <$@> failed ... exiting"
        colorecho 1 "   Look stdout at ${ROOT_DIR}/last_command.out and stderr at ${ROOT_DIR}/last_command.err for details"
        exit 1
    fi
}


function build_binaries {
    # build basesearch binaries with options specified
    SRCDIR=$1
    DSTDIR=$2
    BUILD_FLAGS=$3
    VARIANT=$4

    (
        cd ${SRCDIR};
        wrap_run ya make -j0 --checkout ${BUILD_FLAGS} search/daemons/basesearch tools/dolbilo/dumper tools/dolbilo/executor
        wrap_run ya make -r ${BUILD_FLAGS} search/daemons/basesearch tools/dolbilo/dumper tools/dolbilo/executor

        if [ ! -z "${VARIANT}" ]; then
            DSTDIR="${DSTDIR}/${VARIANT}"
        fi
        mkdir -p ${DSTDIR}

        cp search/daemons/basesearch/basesearch tools/dolbilo/dumper/d-dumper tools/dolbilo/executor/d-executor ${DSTDIR}
    )

}

function download_database {
    TIER_NAME=$1
    echo "Getting ${TIER_NAME} database ..."
    DATABASE_SHARD_NAME=`curl -s "https://ctrl.clusterstate.yandex-team.ru/web/prod-man/view/deploy_progress" | jq 'keys[]' | grep "primus-${TIER_NAME}" | head -1 | sed 's/"//g'`
    echo "    Selected shard ${DATABASE_SHARD_NAME}"
    echo "    Getting iss_shards utility ..."
    ISS_SHARDS_TORRENT=`curl -s "https://sandbox.yandex-team.ru:443/api/v1.0/resource?limit=1&type=ISS_SHARDS&state=READY" | jq '.items[0].skynet_id' | sed 's/"//g'`
    ISS_SHARDS_LOCAL_FILE=`sky files --json "${ISS_SHARDS_TORRENT}" | jq '.[0].name' | sed 's/"//g'`
    wrap_run sky get "${ISS_SHARDS_TORRENT}"
    DATABASE_TORRENT=`./${ISS_SHARDS_LOCAL_FILE} info --json ${DATABASE_SHARD_NAME} 2>/dev/null | jq '.full.urls[0]' | sed 's/"//g'`
    rm "${ISS_SHARDS_LOCAL_FILE}"
    unset ISS_SHARDS_TORRENT
    unset ISS_SHARDS_LOCAL_FILE
    echo "    Downloading database ..."
    mkdir "${BUNDLE_DIR}/database.${TIER_NAME}"
    wrap_run sky get -d "${BUNDLE_DIR}/database.${TIER_NAME}" ${DATABASE_TORRENT}
    unset DATABASE_SHARD_NAME
    unset DATABASE_TORRENT
}

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
BUNDLE_DIR="${ROOT_DIR}/bundle"
ARCADIA_DIR="${ROOT_DIR}/arcadia"

# ============================================== BUILD BINARIES ===============================================

echo "Cloning perftest scripts to ${BUNDLE_DIR} ..."
wrap_run svn export svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/gencfg/utils/other/perftest ${BUNDLE_DIR}

echo "Cloning arcadia to ${ARCADIA_DIR} ..."
wrap_run ya clone ${ARCADIA_DIR}

echo "Generating binaries for x86_64 ..."
cd ${ARCADIA_DIR}

echo "    Generating default binaries ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/x86_64" "" ""

echo "    Generating AMD EPYC binaries (-march=znver1) ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/x86_64" "-DCFLAGS=-march=znver1" "znver1"

echo "    Generating Intel Broadwell binaries (-march=broadwell) ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/x86_64" "-DCFLAGS=-march=broadwell" "broadwell"

echo "    Generating Intel Skylake binaries (-march=skylake) ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/x86_64" "-DCFLAGS=-march=skylake" "skylake"

echo "Generating Arm8 binaries ..."

echo "    Generating default binaries ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/arm8" "--target-platform=default-linux-aarch64 --target-platform-flag WERROR=no" ""

echo "    Generating optmized binaries (-march=armv8.1-a -mtune=thunderx2t99) ..."
build_binaries ${ARCADIA_DIR} "${BUNDLE_DIR}/bin/arm8" "--target-platform=default-linux-aarch64 --target-platform-flag WERROR=no -march=armv8.1-a -mtune=thunderx2t99" "thunderx2t99"

# ============================================= GET PLAN ======================================================

echo "Getting dolbilo plan ..."
RESOURCE_TORRENT=`curl -s "https://sandbox.yandex-team.ru:443/api/v1.0/resource?limit=1&type=BASESEARCH_PLAN&state=READY&attrs=%7B%22db_for_search_queries_PlatinumTier0%22%20%3A%20null%7D" | jq '.items[0].skynet_id' | sed 's/"//g'`
RESOURCE_LOCAL_FILE=`sky files --json "${RESOURCE_TORRENT}" | jq '.[0].name' | sed 's/"//g'`
wrap_run sky get -d ${BUNDLE_DIR} ${RESOURCE_TORRENT}
if [ "${RESOURCE_LOCAL_FILE}" != "plan.bin" ]; then
    wrap_run cp "${BUNDLE_DIR}/${RESOURCE_LOCAL_FILE}" "${BUNDLE_DIR}/plan.bin"
fi
unset RESOURCE_TORRENT
unset RESOURCE_LOCAL_FILE

# ============================================= GET Platinum BASE ======================================================
download_database PlatinumTier0
mv ${BUNDLE_DIR}/database.PlatinumTier0 ${BUNDLE_DIR}/database

# ============================================ GET Tier0 BASE ===========================================================
download_database WebTier0

# ============================================= GET MODELS.ARCHIVE ================================================
echo "Gettings models.archive ..."
MODELS_TASK_ID=`curl -s "https://sandbox.yandex-team.ru:443/api/v1.0/task?type=BUILD_DYNAMIC_MODELS&status=RELEASED&descr_re=MODELS_BASE&limit=1" | jq ".items[0].id"`
MODELS_RESOURCE_ID=`curl -s "https://sandbox.yandex-team.ru:443/api/v1.0/task/${MODELS_TASK_ID}/resources" | jq '.items | map( select(.type == "DYNAMIC_MODELS_ARCHIVE")) | .[0].skynet_id' | sed 's/"//g'`
MODELS_LOCAL_FILE=`sky files --json "${MODELS_RESOURCE_ID}" | jq '.[0].name' | sed 's/"//g'`
sky get -d "${BUNDLE_DIR}/database" ${MODELS_RESOURCE_ID}
if [ "${MODELS_LOCAL_FILE}" != "models.archive" ]; then
    cp "${BUNDLE_DIR}/database/${MODELS_LOCAL_FILE}" "${BUNDLE_DIR}/database/models.archive"
fi

# ============================================= GENERATE VALIDATE QUERIES =========================================
echo "Generating validate queries ..."

wrap_run "${BUNDLE_DIR}/bin/x86_64/d-executor" -D -p "${BUNDLE_DIR}/plan.bin" -Q 200 | grep "^GET" | sed 's/GET \/yandsearch?//' | awk '{print $1}' >"${BUNDLE_DIR}/validator.queries"

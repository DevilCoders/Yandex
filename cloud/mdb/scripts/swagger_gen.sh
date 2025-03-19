#!/bin/bash

set -e

search_path=$(pwd)
while [ "$seach_path" != "/" ] ; do
    search_path=$(dirname "$search_path")
    if [[ $(find "$search_path" -maxdepth 1 -name ya) ]]; then
        ARCADIA_ROOT=${search_path}
        break
    fi;
done

echo "Generating swagger code for '${MDB_PROJECT_NAME}'"

if [[ ! -f "api/swagger.yaml" ]]; then
    echo "Swagger spec does not exist. Skipping."
    exit 0
fi

# This var is used for when project name doesn't match binary name (happens with swagger-generated code)
if [[ -z "${MDB_SWAGGER_CMD_NAME}" ]]; then
    export MDB_SWAGGER_CMD_NAME=${MDB_PROJECT_NAME}
fi

GEN_PATH=generated/swagger

mkdir -p ${GEN_PATH}

SWAGGER_BIN="${ARCADIA_ROOT}/ya tool swagger"

$SWAGGER_BIN generate model -f api/swagger.yaml -t ${GEN_PATH}

if [[ ! "${MDB_SWAGGER_NO_SERVER}" == 1 ]]; then
    $SWAGGER_BIN generate server -f api/swagger.yaml -t ${GEN_PATH} --skip-models --name ${MDB_SWAGGER_CMD_NAME} --flag-strategy pflag
    rm -rf cmd/${MDB_PROJECT_NAME}
    mkdir -p cmd
    mv -f ${GEN_PATH}/cmd/${MDB_SWAGGER_CMD_NAME}-server cmd/${MDB_PROJECT_NAME}
    rm -d ${GEN_PATH}/cmd
else
    echo "Swagger server generation disabled."
fi

$SWAGGER_BIN generate client -f api/swagger.yaml -t ${GEN_PATH} --skip-models --name ${MDB_SWAGGER_CMD_NAME}

${ARCADIA_ROOT}/ya tool yo fix -add-owner=g:mdb ${GEN_PATH}
if [[ -d 'cmd' ]]; then
    ${ARCADIA_ROOT}/ya tool yo fix -add-owner=g:mdb cmd
fi

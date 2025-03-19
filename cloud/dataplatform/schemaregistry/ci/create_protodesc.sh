#!/bin/bash

set -exo pipefail

export ARCADIA_ROOT="${PWD}"
export GRPC_PLAINTEXT=1
PATH="${ARCADIA_ROOT}:${PATH}"
PATH="$(dirname $(ya tool go --print-path)):${PATH}"
PROTODESC_GENERATOR_PATH="${ARCADIA_ROOT}/cloud/dataplatform/schemaregistry/cmd/schemaregistryctl"
PROTODESC_GENERATOR_EXECUTABLE_PATH="${PROTODESC_GENERATOR_PATH}/schemaregistryctl"

ya make "${PROTODESC_GENERATOR_PATH}"
ya make "tasklet/registry/${NAMESPACE}"

set +e
"${PROTODESC_GENERATOR_EXECUTABLE_PATH}" \
    extract \
    --input="${ARCADIA_ROOT}/tasklet/registry/${NAMESPACE}/tasklet-registry-${NAMESPACE}.protodesc" \
    --output="${ARCADIA_ROOT}/tasklet/registry/${NAMESPACE}/desc"

cp -r "${ARCADIA_ROOT}/tasklet/registry/${NAMESPACE}/desc/" "${RESULT_RESOURCES_PATH}/descriptors"

"${PROTODESC_GENERATOR_EXECUTABLE_PATH}" \
    schema bulk-create \
    --comp=${COMPATIBILITY:-COMPATIBILITY_BACKWARD} \
    --namespace=${NAMESPACE} \
    --format=${FORMAT:-FORMAT_PROTOBUF} \
    --glob=$"${ARCADIA_ROOT}/tasklet/registry/${NAMESPACE}/desc/*.desc" \
    --host=${REGISTRY:-schema-registry-testing.yandex-team.ru:8443} \
    --output=${RESULT_RESOURCES_PATH}/report

status=$?

[ $status -eq 0 ] && echo "Upload was successful" || echo "Upload failed"

exit $status

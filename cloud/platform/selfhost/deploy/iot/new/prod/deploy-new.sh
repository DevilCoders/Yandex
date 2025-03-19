#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
source common.sh

####### CREATE INSTANCE-GROUP
set -x
yc compute instance-group create --folder-id=${FOLDER_ID} --file ${SPEC_RENDERED} --async --format=json --endpoint ${API_ENDPOINT}
set +x

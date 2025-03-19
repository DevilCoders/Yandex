#!/usr/bin/env bash

set -ex

VERSION=$(date +%Y-%m-%dT%H-%M)
CURRENT_DIR=`dirname $0`
ARCADIA_DIR=${CURRENT_DIR}/../../../../
cd ${ARCADIA_DIR}
svn update
DOCKER_DIR=./cloud/kms/docker/java-ydb

ya make --checkout --target-platform=linux ./kikimr/public/tools/ydb
cp ./kikimr/public/tools/ydb/ydb ${DOCKER_DIR}

docker build -f ${DOCKER_DIR}/Dockerfile -t java-ydb:${VERSION} ${DOCKER_DIR}

docker tag java-ydb:${VERSION} cr.yandex/crp6ro8l0u0o3qgmvv3r/java-ydb:${VERSION}
docker push cr.yandex/crp6ro8l0u0o3qgmvv3r/java-ydb:${VERSION}

echo "cr.yandex/crp6ro8l0u0o3qgmvv3r/java-ydb:${VERSION}" > ${DOCKER_DIR}/image.txt

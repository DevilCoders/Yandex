#!/usr/bin/env bash

set -ex

VERSION=$(date +%Y-%m-%dT%H-%M)
DOCKER_DIR=./cloud/trail/docker/java-tvm

docker build -f ${DOCKER_DIR}/Dockerfile -t java-tvm:${VERSION} ${DOCKER_DIR}
docker tag java-tvm:${VERSION} cr.yandex/crpij8i2mnr7bpbup1c5/java-tvm:${VERSION}
docker push cr.yandex/crpij8i2mnr7bpbup1c5/java-tvm:${VERSION}

echo "cr.yandex/crpij8i2mnr7bpbup1c5/java-tvm:${VERSION}" > ${DOCKER_DIR}/image.txt

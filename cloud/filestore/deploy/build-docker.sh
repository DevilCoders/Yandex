#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" == "prod" ]]; then
    DOCKER_REGISTRY="cr.yandex/crpsohroskvc81pevas8"
elif [[ "$PROFILE" == "preprod" ]]; then
    DOCKER_REGISTRY="cr.cloud-preprod.yandex.net/crto6heesjan2mj2oq5h"
else
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

DOCKER_DIR="$(dirname "$0")/docker"

for pkg in yandex-cloud-filestore-{vhost,server,http-proxy,nfs}; do
    ya package "$DOCKER_DIR/$pkg/pkg.json" \
        --docker \
        --docker-push \
        --docker-registry="$DOCKER_REGISTRY" \
        $@
done

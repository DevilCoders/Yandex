#!/bin/bash

NUPKG_BUILDER_IMAGE=${NUPKG_BUILDER_IMAGE:-registry.yandex.net/dbaas/nupkg-builder:latest}

docker build --no-cache --tag=$NUPKG_BUILDER_IMAGE .
docker push $NUPKG_BUILDER_IMAGE

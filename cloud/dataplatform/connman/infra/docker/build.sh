#!/bin/bash

set -e

CONFIG_FILE="config_$PROFILE.yaml"
PACKAGE_FILE="pkg_$PROFILE.json"
(
  export CONFIG_FILE
  envsubst <pkg.json >"$PACKAGE_FILE"
)

ya package "$PACKAGE_FILE" \
  --docker \
  --docker-registry "$DOCKER_REGISTRY" \
  --docker-repository "$DOCKER_REPOSITORY" \
  --docker-push \
  --docker-build-arg CONFIG_FILE="$CONFIG_FILE"

rm "$PACKAGE_FILE"

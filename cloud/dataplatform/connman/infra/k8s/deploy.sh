#!/bin/bash

set -e

if [ -z "$DOCKER_IMAGE" ]; then
  echo "DOCKER_IMAGE must be set"
  exit 1
fi

APP_FILE="app_$PROFILE.yaml"
envsubst <app.yaml >"$APP_FILE"

source setup_kubectl.sh

kubectl apply -f "$APP_FILE" --context "$K8S_CONTEXT"

rm "$APP_FILE"

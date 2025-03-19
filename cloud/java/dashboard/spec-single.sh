#!/bin/sh -e

ACTION=${1:-local}
SCOPE=${2:-test}
SPEC_FILE=${3:-demo.yaml}

SCRIPT_DIR="$( cd "$(dirname "$0")" ; pwd -P )" #"
DIR=$SCRIPT_DIR/src/$SCOPE/resources/dashboard

IMG=registry.yandex.net/cloud/platform/dashboard:latest
#IMG=cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest
JAR=java-dashboard.jar

docker run --rm -it -v $DIR:/data/ \
  -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" \
  -e WRITE_INPUT=false \
  -e WRITE_OUTPUT=false \
  $IMG java -jar build/$JAR \
  $ACTION data/$SPEC_FILE

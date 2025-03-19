#!/bin/sh
#
# Build docker image.
#
# Usually run once.
# Re-run needed only if yc-solomon-cli version or Dockerfile changed.
#
# When image is built, run it using ./docker-run.sh

cd $(dirname $0)
DOCKER_IMAGE_NAME=yc-monitoring
docker build --network host --tag $DOCKER_IMAGE_NAME --file ./Dockerfile .

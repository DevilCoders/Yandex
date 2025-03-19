#!/bin/bash

DOCKER_ID=`docker ps --format '{{.ID}}' --filter label=io.kubernetes.container.name=lockbox-tool`
docker exec -it $DOCKER_ID tool $*

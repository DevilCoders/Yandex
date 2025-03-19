#!/bin/bash

DOCKER_ID=`docker ps --format '{{.ID}}' --filter label=io.kubernetes.container.name=certificate-manager-ydb-dumper`
docker exec -it $DOCKER_ID ydb-dumper $*

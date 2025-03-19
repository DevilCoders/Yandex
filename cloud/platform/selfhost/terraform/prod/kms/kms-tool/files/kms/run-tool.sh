#!/bin/bash

DOCKER_ID=`docker ps --format '{{.ID}}' --filter label=io.kubernetes.container.name=kms-tool`
docker exec -it $DOCKER_ID java -jar maven/kms-tool-${tool_version}.jar $*

#!/bin/bash

DOCKER_ID=`docker ps --format '{{.ID}}' --filter label=io.kubernetes.container.name=trail-tool`
docker exec -it $DOCKER_ID java -jar maven/trail-tool-${application_version}.jar $*

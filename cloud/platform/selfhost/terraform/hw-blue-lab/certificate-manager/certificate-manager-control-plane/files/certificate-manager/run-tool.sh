#!/bin/bash

DOCKER_ID=`docker ps --format '{{.ID}}' --filter label=io.kubernetes.container.name=certificate-manager-tool`
docker exec -it $DOCKER_ID java -jar maven/certificate-manager-tool-${tool_version}.jar $*

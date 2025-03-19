#!/bin/bash

set -x
set -e

DATE=$(date -d'now-12 hours' '+%F %T %z %Z')
RUNNING_CONTAINERS=$(docker ps --format '{{.ID}},{{.CreatedAt}}' | awk -vDate="$DATE" -F, '{ if (Date > $2) print $1 }')
if [[ -n "$RUNNING_CONTAINERS" ]]; then
    docker rm --force --volumes $RUNNING_CONTAINERS
fi

docker container prune --force --filter until=1h

VOLUMES=$(docker volume ls --quiet --filter dangling=true)
if [[ -n "$VOLUMES" ]]; then
    docker volume rm --force $VOLUMES
fi

/opt/tox/bin/python /usr/local/bin/docker_images_prune.py

for directory in $(ls /home/robot-pgaas-ci/workspace/ | grep ws-cleanup)
do
    rm -rf "/home/robot-pgaas-ci/workspace/${directory}"
done

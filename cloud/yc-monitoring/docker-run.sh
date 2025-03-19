#!/bin/bash
#
# Usage: ./docker-run.sh [ARGS]
#
# If ARGS not specified, runs interactive bash session.

DOCKER_IMAGE_NAME=yc-monitoring

CMD="${1:-bash}"
shift

if [ -t 0 ]      # If STDIN is a terminal
  then IT="-it"
  else IT=""
fi

docker run \
    $IT \
    --rm \
    --network host \
    --volume "$(pwd):/root/project:ro" \
    --env JUGGLER_OAUTH_TOKEN=$JUGGLER_OAUTH_TOKEN \
    --env SOLOMON_OAUTH_TOKEN=$SOLOMON_OAUTH_TOKEN \
    --env SOLOMON_IAM_TOKEN=$SOLOMON_IAM_TOKEN \
    --env TEAMCITY=$TEAMCITY \
    --env FORCE_SYNC_MENU=$FORCE_SYNC_MENU \
    -- \
    $DOCKER_IMAGE_NAME \
    $CMD \
    "$@"

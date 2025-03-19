#!/bin/bash

DOCKERFILE_DIR="$(dirname "$0")"
TAG="latest"
if [[ ! -z "$1" ]]
then
    TAG=$1
fi
echo "cr.yandex/crppns4pq490jrka0sth/logrotate:$TAG"
docker build $DOCKERFILE_DIR -t "cr.yandex/crppns4pq490jrka0sth/logrotate:$TAG"

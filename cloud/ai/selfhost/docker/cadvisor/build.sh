#!/bin/bash

TAG="latest"
if [[ ! -z "$1" ]]
then
    TAG=$1
fi
CONTAINER_NAME="cr.yandex/crppns4pq490jrka0sth/cadvisor:${TAG}"
echo ${CONTAINER_NAME}

docker build -t ${CONTAINER_NAME} --build-arg version=${TAG} .
docker push ${CONTAINER_NAME}

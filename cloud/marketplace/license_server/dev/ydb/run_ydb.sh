#!/usr/bin/env bash

YDB_HOST=localhost
HOSTNAME=`hostname`
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
YDB_CA_CERT_PATH=${SCRIPTPATH}/ydb_ca.pem
YDB_CONTAINER_NAME=ydb

if [ -x "$(command -v docker-machine)" ]; then
  docker-machine start || true
  eval $(docker-machine env)
  YDB_HOST=localhost
fi
if [[ $HOSTNAME == *'.yp-c.yandex.net'* ]]; then
  YDB_HOST=$HOSTNAME
  echo "We are on QYP machine"
  echo "Don't forget to change local configs to match hostname"
fi
#image_name=registry.yandex.net/yandex-docker-local-ydb@sha256:37708f9123772776c25bf2da101ccb0b9e0e74c9c5dbd20ac93aa85dbb2a746b
#image_name=registry.yandex.net/yandex-docker-local-ydb:stable
image_name=cr.yandex/yc/yandex-docker-local-ydb

docker stop ydb -t0 &>/dev/null || true
docker pull ${image_name}
docker run --hostname ${YDB_HOST} -d -p 2135:2135 -p 8765:8765 --name ${YDB_CONTAINER_NAME} --ulimit nofile=90000:90000 --env 'YDB_YQL_SYNTAX_VERSION=1' --rm $image_name

echo "YDB Server listening on ${YDB_HOST}:2135"
echo "YDB Viewer on http://${YDB_HOST}:8765"

# Wait until certs are availabe and copy localy
until docker cp ${YDB_CONTAINER_NAME}:/ydb_certs/ca.pem ${YDB_CA_CERT_PATH}; do
  echo "Retrying..."
  sleep 2
done

echo "YDB CA_CERT available at ${YDB_CA_CERT_PATH}"
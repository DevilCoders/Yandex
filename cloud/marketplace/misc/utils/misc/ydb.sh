#!/usr/bin/env bash

YDB_HOST=localhost

if [ -x "$(command -v docker-machine)" ]; then
  docker-machine start || true
  eval $(docker-machine env)
  YDB_HOST=local-ydb
fi

docker stop ydb  &>/dev/null || true
docker run --hostname ${YDB_HOST} -dp 2135:2135 -p 8765:8765 --name ydb --ulimit nofile=90000:90000  -d --rm registry.yandex.net/yandex-docker-local-ydb:stable

echo "YDB Server listening on ${YDB_HOST}:2135"
echo "YDB Viewer on http://${YDB_HOST}:8765"


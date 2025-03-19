#!/bin/sh

action=${1}
path=${2}
dir=$(realpath "${path}")
dir=$(dirname "${dir}")
file=$(basename "${path}")
docker run --rm -it \
  -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" \
  -v "${dir}":/data/ \
  registry.yandex.net/cloud/platform/dashboard:latest \
  java -jar build/java-dashboard.jar ${action} /data/${file}
filename="${file%.*}"
if [[ "${action}" == "upload" ]]; then
  rm "${dir}/${filename}.json"
fi

#!/bin/bash

docker run --rm -it \
-v $(pwd):/data/ \
-e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" \
-e DASHBOARD_ACTION="upload" \
-e DASHBOARD_SPEC="/data/$1" \
cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest

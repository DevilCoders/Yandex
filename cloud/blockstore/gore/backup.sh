#!/usr/bin/env bash

curl -v -k -H "Authorization: OAuth $TOKEN" "https://resps-api.cloud.yandex.net/api/v0/services/nbs" > nbs.json

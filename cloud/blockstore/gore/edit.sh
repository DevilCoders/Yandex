#!/usr/bin/env bash
set -x 
update_gore() {
    local patch_file="$1"
    curl -v -k -H "Authorization: OAuth $TOKEN" -XPATCH -T "./${patch_file}" "https://resps-api.cloud.yandex.net/api/v0/services/nbs"
}

#update_gore nbs.patch.01-teamowners.json
update_gore nbs.patch.02-schedule_abc.json

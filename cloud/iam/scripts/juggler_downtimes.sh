#!/usr/bin/env bash
# shellcheck disable=SC2155

set -e

# https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0
JUGGLER_OAUTH_TOKEN_FILE=${JUGGLER_OAUTH_TOKEN_FILE:=~/.config/oauth/juggler}
JGLR_OAUTH_TOKEN=${JUGGLER_OAUTH_TOKEN:-$(cat ${JUGGLER_OAUTH_TOKEN_FILE?})}
JGLR_AUTH="Authorization: OAuth $JGLR_OAUTH_TOKEN"
JGLR_URL=https://juggler-api.search.yandex.net/v2/downtimes

# Advanced filters could be applied in the following manner:
#   add_downtime "$(dt_filters host my-service.svc.cloud.yandex.net service reboot-count)" 180
function add_downtime {
  local dt_filter=${1:?"Usage: add_downtime <host or filter json> [no of minutes]
         Example: add_downtime iam-myt1.svc.cloud.yandex.net 5"}
  local dt_minutes=${2:-60}
  dt_until=$(($(date +"%s") + dt_minutes * 60))
  if [[ "${dt_filter?}" == \{* ]];
  then
    local jglr_req=$(jq -c --null-input \
      --arg end_time $dt_until \
      --argjson filter "${dt_filter?}" \
      '{end_time:$end_time, filters: [ $filter ]}')
  else
    local jglr_req=$(jq -c --null-input \
      --arg end_time $dt_until \
      --arg host "${dt_filter?}" \
      '{end_time:$end_time, filters: [ {host:$host} ]}')
  fi;

  local jglr_resp=$(curl -s -XPOST -H"$JGLR_AUTH" -H'Content-Type: application/json' "$JGLR_URL/set_downtimes" --data-raw "$jglr_req")
  CURRENT_DT=$(echo $jglr_resp | jq '.downtime_id' -r)
  echo "downtime ${CURRENT_DT?} set for ${dt_filter?} for $dt_minutes minutes; url: https://juggler.yandex-team.ru/downtimes/?query=downtime_id%3D${CURRENT_DT?}"
}

function remove_downtime {
  local dt_id=${1:-$CURRENT_DT}
  local dt_id=${dt_id:?"provide DT id"}
  local jglr_req=$(jq -c --null-input --arg dt_id $dt_id '{downtime_ids: [ $dt_id ]}')
  echo "removing downtime $dt_id"
  local jglr_resp=$(curl -s -XPOST -H"$JGLR_AUTH" -H'Content-Type: application/json' "$JGLR_URL/remove_downtimes" --data-raw "$jglr_req")
  local removed_dt_id=$(echo $jglr_resp | jq '.downtimes[0].downtime_id')
  echo "downtime removed: $removed_dt_id"
}

# Builds filters json:
#   dt_filters host host123 service reboot-count
#       => {"host":"host123","service":"reboot-count"}
# Intended to be used with add_downtime:
#   add_downtime "$(dt_filters host host123 service reboot-count)"
function dt_filters() {
    local json='{}'
    while [[ $# -gt 0 ]];
    do
        json=$(echo "${json?}" | jq -c \
            --arg param "$1" \
            --arg value "$2" \
            '.[$param] = $value')
        shift # processed param
        shift # processed value
    done
    echo "${json?}"
}

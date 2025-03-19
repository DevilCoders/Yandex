#!/usr/bin/env bash
set -e
ST_OAUTH_TOKEN=${ST_OAUTH_TOKEN:-$(cat ${ST_OAUTH_TOKEN_FILE:?"Either ST_OAUTH_TOKEN or ST_OAUTH_TOKEN_FILE env variable is required"})}
ST_AUTH="Authorization: OAuth $ST_OAUTH_TOKEN"
ST_URL=https://st-api.yandex-team.ru/v2/issues

ticket=${1:?"1st argument - ticket id"}
comment=${2:?"2nd argument - comment text"}

req=$(jq -c --null-input --arg comment "$comment" '{text:$comment}')
st_resp=$(curl -s -XPOST -H"$ST_AUTH" -H'Content-Type: application/json' "$ST_URL/$ticket/comments" --data-raw "$req")
echo "added ST comment: https://st.yandex-team.ru/${ticket}#$(echo $st_resp | jq -cr '.longId')"

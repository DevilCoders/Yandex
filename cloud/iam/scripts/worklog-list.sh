#!/bin/bash
TICKET=$1

if [[ -z ${TICKET} ]]; then
  echo "Usage: $0 <ticket>"
  exit 0
fi

if [[ -z ${OAUTH_TOKEN} ]]; then
  OAUTH_TOKEN=$(cat ${HOME}/.config/oauth/st)
fi

curl -s -H "authorization: OAuth ${OAUTH_TOKEN}" \
  -XGET https://st-api.yandex-team.ru/v2/issues/${TICKET}/worklog?pretty |
  jq -rc ".[] | {id: .id, start: .start, time: .duration}"
  

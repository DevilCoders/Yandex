#!/bin/bash
TICKET=$1
ENTRY=$2

if [[ -z ${ENTRY} ]]; then
  echo "Usage: $0 <ticket> <entry>"
  exit 0
fi

if [[ -z ${OAUTH_TOKEN} ]]; then
  OAUTH_TOKEN=$(cat ${HOME}/.config/oauth/st)
fi

curl -s -H "authorization: OAuth ${OAUTH_TOKEN}" \
  -XDELETE https://st-api.yandex-team.ru/v2/issues/${TICKET}/worklog/${ENTRY}
  

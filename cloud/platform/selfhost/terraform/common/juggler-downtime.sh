#!/usr/bin/env bash


: ${JUGGLER_TOKEN:?no oauth token defined; get one here https://nda.ya.ru/3UYFuV}

if [ $# -lt 2 ]
then
    echo "Usage: $0 <on|off> <ipv6 address|downtime_id>"
    exit 1
fi

if [ ${1} == "on" ] ; then
if [[ ${2} =~ ":" ]] ; then
HOSTNAME=$(dig -x ${2} +short)
else
HOSTNAME="${2}"
fi
timeout_min="${JUGGLER_TIMEOUT_MINUTES:-3}"

output=$(curl -X POST -s "https://juggler-api.search.yandex.net/v2/downtimes/set_downtimes" \
  -H "Authorization: OAuth ${JUGGLER_TOKEN}" \
  -H "Accept: application/json" \
  -H "Content-Type: application/json" \
  -d @- << EOF
{
  "description": "terraform deploy",
  "end_time": $(( $(date +%s) + ${timeout_min}*60)),
  "filters": [
    {
      "host": "${HOSTNAME%.}"
    }
  ],
  "start_time": 0
}
EOF
)

if [ $# -ge 3 ] && [ "${3}" == "--raw-id" ] ; then
echo -ne ${output} | jq -r .downtime_id
else
echo "Downtime ID is $(echo -ne ${output} | jq -r .downtime_id)"
fi

fi # if [ ${1} == "on" ] ;

if [ ${1} == "off" ] ; then
DOWNTIME_ID=${2}
curl -X POST -s "https://juggler-api.search.yandex.net/v2/downtimes/remove_downtimes" \
  -H "Authorization: OAuth ${JUGGLER_TOKEN}" \
  -H "Accept: application/json" \
  -H "Content-Type: application/json" \
  -d @- << EOF
{
  "downtime_ids": [
    "${DOWNTIME_ID}"
  ]
}
EOF
fi

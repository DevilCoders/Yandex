#!/bin/bash
set -e

DATABASE=/pre-prod_global/iam
ENDPOINT_SUFFIX=cloud-preprod.yandex.net
YDB_ENDPOINT="grpcs://ydb-iam.${ENDPOINT_SUFFIX}:2136"

function refresh_reaper_token_file() {
  curl -sS -H "Metadata-Flavor: Google" http://localhost:6770/computeMetadata/v1/instance/service-accounts/default/token \
    | jq -r .access_token | tr -d '\n' > /var/lib/autodelete-token
}

date

refresh_reaper_token_file
/usr/local/bin/yc-iam-autodelete -e ${YDB_ENDPOINT} -d ${DATABASE} \
  -r ${DATABASE}/hardware/default/identity/r3/ -o /var/lib/autodelete-clouds.json -a /var/lib/autodelete-token \
  -p 58 -s BLOCKED BLOCKED_BY_BILLING BLOCKED_MANUALLY -l 2000

tr \' \" < /var/lib/autodelete-clouds.json | jq -crs .[].id | /usr/local/bin/delete-blocked-clouds.sh

# https://st.yandex-team.ru/CLOUD-56921
#refresh_reaper_token_file
#/usr/local/bin/yc-iam-autodelete -e ${YDB_ENDPOINT} -d ${DATABASE} \
#  -r ${DATABASE}/hardware/default/identity/r3/ -o /var/lib/autodelete-clouds.json -a /var/lib/autodelete-token \
#  -p 58 -l 4000
#
#tr \' \" < /var/lib/autodelete-clouds.json | jq -crs .[].id | /usr/local/bin/delete-blocked-clouds.sh

rm /var/lib/autodelete-token

echo done.

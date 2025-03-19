#!/usr/bin/env bash

set -e

if [[ -z "${YAV_TOKEN}" ]]; then
    export YAV_TOKEN=$(yav oauth --rsa-private-key ~/.ssh/id_rsa --rsa-login $(awk '{split($3,a,"@"); print a[1]; exit}' ~/.ssh/id_rsa.pub))
else
    export YAV_TOKEN="${YAV_TOKEN}"
fi

cloud_token=$(yav get version sec-01cxfsdq7tpyyg4r16x9v57z6w -o oauth)
docker_auth=$(yav get version sec-01cxfqnhgdyev0bkk17vtm0dwv -o docker-auth)

jq -n \
--arg docker_auth "$docker_auth" \
--arg cloud_token "$cloud_token" \
'{
    "docker_auth"   :$docker_auth,
    "cloud_token"   :$cloud_token
}'

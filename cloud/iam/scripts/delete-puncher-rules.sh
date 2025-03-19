#!/bin/bash
set -e

SERVICE_NAME=${1}
TICKET=${2}

if [[ -z ${TICKET} ]]; then
 echo "Usage: $0 <prefixes> ticket"
 echo "Example: $0 identity,scms CLOUD-666"
 exit 0
fi

delete_rules() {
  suffix=$1
  prefixes=(${SERVICE_NAME//,/ })
  name=$hosts
  for prefix in "${prefixes[@]}"; do
    rules=$(puncher get --json-request --exact --destination "${prefix}.${suffix}" 2>/dev/null || true)
    if [[ ! -z "${rules}" ]]; then
      jq . <<< "${rules}"
      read -p "Delete [Y/n]? " -n 1 -r
      if [[ ${REPLY:-y} =~ ^[Yy]$ ]]; then
        jq -c '.destinations = []' <<< "${rules}" |
          puncher create --multiple --comment "${TICKET} remove ${prefix}.${suffix}"
      fi
    fi
  done
}

delete_rules cloud.yandex-team.ru
delete_rules prestable.cloud-internal.yandex.net
delete_rules dev.cloud-internal.yandex.net

delete_rules private-api.cloud-testing.yandex.net
delete_rules private-api.cloud-preprod.yandex.net
delete_rules private-api.cloud.yandex.net

delete_rules private-api.gpn.yandexcloud.net
delete_rules private-api.private-testing.yandexcloud.net

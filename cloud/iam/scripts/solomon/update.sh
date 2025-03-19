#!/bin/bash
#set -x
set -e

BASE_DIR="$(dirname $(dirname $0))"
CONFIG_FILE="${BASE_DIR}/config.json"

USAGE="usage: $0 <ENVIRONMENT>"

if [[ -z "$1" ]]; then
    echo "Environment required!"
    echo ${USAGE}
    exit 1
fi
export ENV_ID="$1"

DRY_RUN=0
if [[ ${2} == "--dry-run" ]]; then
    DRY_RUN=1
fi

JSON_ENV=$(jq ".environments[] | select(.id == \"${ENV_ID}\")" ${CONFIG_FILE})

if [[ -z ${JSON_ENV} ]]; then
    echo "Environment must be one of: [ $(jq -r '[.environments[].id] | join(" | ")' ${CONFIG_FILE}) ]"
    echo ${USAGE}
    exit 1
fi

if [[ -z ${IAM_TOKEN} ]]; then
    YC_ENV=$(echo ${JSON_ENV} | jq -r ".ycEnv")
    IAM_TOKEN=$(ycp --profile=${YC_ENV} iam create-token)
fi

export PROJECT_ID=$(echo ${JSON_ENV} | jq -r ".projectId")
export SOLOMON_ENDPOINT=$(echo ${JSON_ENV} | jq -r ".endpoint")

function solomon {
  path=$1
  method=$2
  if [[ $DRY_RUN == 1 ]]; then
    echo "update solomon skipped in dry-run mode"
  else
  curl --silent -X ${method} \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    -H "Authorization: Bearer ${IAM_TOKEN}" \
    -d @- https://${SOLOMON_ENDPOINT}/api/v2/projects/${PROJECT_ID}${path} \
    | jq
  fi
}

function solomon_file {
  file=$1
  path=$2

  entityId=$(envsubst < ${file} | jq -r '.id')

  existingEntity=$(curl --silent -X GET \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    -H "Authorization: Bearer ${IAM_TOKEN}" \
    "https://${SOLOMON_ENDPOINT}/api/v2/projects/${PROJECT_ID}${path}${entityId}")

  entityVersion=$(echo ${existingEntity} | jq -r '.version')

  if [[ ${entityVersion} != 'null' ]]; then
    # version is NOT null then check existing
    creatingEntity=$(envsubst < ${file} | jq -S ".")
    affectingKeys=$(echo ${creatingEntity} | jq -r "keys | join(\",\")")
    existingEntity=$(echo ${existingEntity} | jq -S "{${affectingKeys}}")

    if [[ "${existingEntity}" == "${creatingEntity}" ]]; then
      echo Not changed: ${path}${entityId} version ${entityVersion}.
    else
      # then UPDATE existing
      echo CHANGED: ${path}${entityId} version ${entityVersion} is changed. Update it.
      echo "Existing version:"
      echo "${existingEntity}" | jq
      echo "Modified version:"
      echo "${creatingEntity}" | jq
      envsubst < ${file} | jq ".version |= ${entityVersion}" | solomon "${path}${entityId}" "PUT"
    fi
  else
    # version is null then CREATE new
    echo NOT EXIST: ${path}${entityId} . Create new
    envsubst < ${file} | solomon "${path}" "POST"
  fi
}

function update_services {
# update services
for SERVICE_NAME in "${SERVICES_ARRAY[@]}"; do
  export SERVICE_NAME
  JSON_SERVICE=$(echo ${JSON_SERVICES} | jq ".[] | select(.name == \"${SERVICE_NAME}\")")
  export SERVICE_SOLOMON_AGENT_PATH=$(echo ${JSON_SERVICE} | jq -r ".solomonAgentPath")
  export SENSOR_NAME_LABEL=$(echo ${JSON_SERVICE} | jq -r ".sensorNameLabel")
  templateFile=$(echo ${JSON_SERVICE} | jq -r ".customTemplate")
  if [[ ${templateFile} == 'null' ]]; then
    if [[ ${SERVICE_SOLOMON_AGENT_PATH} == 'null' ]]; then
      templateFile="service-push.json"
    else
      templateFile="service-pull.json"
    fi
  fi

  solomon_file "${BASE_DIR}/templates/${templateFile}" /services/
done
}

function update_clusters {
# update clusters
for CLUSTER_NAME in "${CLUSTERS_ARRAY[@]}"; do
  export CLUSTER_NAME
  JSON_CLUSTER=$(echo ${JSON_CLUSTERS} | jq ".[] | select(.name == \"${CLUSTER_NAME}\")")
  export CLUSTER_ID=$(echo ${JSON_CLUSTER} | jq -r ".folderId")
  if [[ $(echo ${JSON_CLUSTER} | jq '.conductorGroups') != 'null' ]]; then
    export CONDUCTOR_GROUPS=$(echo ${JSON_CLUSTER} | jq '.conductorGroups | map({"group": ., "labels": []})')
  else
    export CONDUCTOR_GROUPS=null
  fi
  if [[ $(echo ${JSON_CLUSTER} | jq '.cloudDns') != 'null' ]]; then
    ENV_UPPER="$(echo ${ENV_ID?} | tr [:lower:] [:upper:] )"
    export CLOUD_DNS=$(echo ${JSON_CLUSTER} | jq ".cloudDns | map({\"env\": \"${ENV_UPPER?}\", \"name\": ., \"labels\": []})")
  else
    export CLOUD_DNS=null
  fi
  export CLUSTER_USE_FQDN=$(echo ${JSON_CLUSTER} | jq -r '.useFqdn // false')

  solomon_file ${CLUSTER_FILE} /clusters/
done
}

function update_shards {
# update shards
for CLUSTER_NAME in "${CLUSTERS_ARRAY[@]}"; do
  export CLUSTER_ID=$(echo ${JSON_CLUSTERS} | jq -r ".[] | select(.name == \"${CLUSTER_NAME}\").folderId")
  for SERVICE_NAME in "${SERVICES_ARRAY[@]}"; do
    export SERVICE_NAME
    JSON_SERVICE=$(echo ${JSON_SERVICES} | jq ".[] | select(.name == \"${SERVICE_NAME}\")")
    SERVICE_CLUSTER=$(echo ${JSON_SERVICE} | jq -r ".clusters[] | select(. == \"${CLUSTER_NAME}\")")
    if [[ -n ${SERVICE_CLUSTER} ]]; then
      solomon_file ${SHARD_FILE} /shards/
    fi
  done
done
}

function update_channels {
# update channels
for CHANNEL_FILE in ${BASE_DIR}/channels/*.json; do
  solomon_file "${CHANNEL_FILE}" /notificationChannels/
done
}

function update_alerts {
# update alerts
for SERVICE_NAME in "${SERVICES_ARRAY[@]}"; do
  export SERVICE_NAME
  JSON_SERVICE=$(echo ${JSON_SERVICES} | jq ".[] | select(.name == \"${SERVICE_NAME}\")")
  JSON_ALERTS=$(echo ${JSON_SERVICE} | jq ".alerts")
  JSON_CROSS_SERVICES=$(echo ${JSON_SERVICE} | jq ".cross_services")
  ALERTS_ARRAY=( $(echo ${JSON_ALERTS} | jq -r '.? | keys | .[]') )
  for ALERT in "${ALERTS_ARRAY[@]}"; do
    JSON_ALERT=$(echo ${JSON_ALERTS} | jq ".\"${ALERT}\"")
    for CLUSTER_NAME in "${CLUSTERS_ARRAY[@]}"; do
      JSON_ALERT_FOR_CLUSTER=$(echo ${JSON_ALERT} | jq -r ".\"${CLUSTER_NAME}\"")
      if [[ ${JSON_ALERT_FOR_CLUSTER} != 'null' ]]; then
        export CLUSTER_NAME

        JSON_CHANNELS=$(echo ${JSON_ALERT_FOR_CLUSTER} | jq -r ".\"channels\"")
        if [[ ${JSON_CHANNELS} != 'null' ]]; then
          CHANNELS=$(echo ${JSON_CHANNELS} | jq -r "map({\"id\": ., \"config\": {}})")
        else
          CHANNELS="[]"
        fi
        export CHANNELS

        DATASOURCE=$(echo ${JSON_ALERT_FOR_CLUSTER} | jq -r ".\"datasource\"")
        export DATASOURCE

        CLUSTER_SUFFIX=$(echo ${JSON_ALERT_FOR_CLUSTER} | jq -r ".\"cluster_suffix\"")
        export CLUSTER_SUFFIX

        if [[ ${JSON_CROSS_SERVICES} == 'null' ]]; then
          solomon_file "${BASE_DIR}/templates/alerts/${ALERT}.json" /alerts/
        else
          CROSS_SERVICES_ARRAY=( $(echo ${JSON_CROSS_SERVICES} | jq -r '.[]') )
          for SERVICE in "${CROSS_SERVICES_ARRAY[@]}"; do
            export SERVICE
            solomon_file "${BASE_DIR}/templates/alerts/${ALERT}.json" /alerts/
          done
        fi
      fi
    done
  done
done

}

function update_all {
  update_services
  update_clusters
  update_shards
  update_channels
  update_alerts
}

CLUSTER_FILE="${BASE_DIR}/templates/cluster.json"
SHARD_FILE="${BASE_DIR}/templates/shard.json"

JSON_SERVICES=$(jq '[.services[]]' ${CONFIG_FILE})
JSON_CLUSTERS=$(echo ${JSON_ENV} | jq "[.clusters[]]")

SERVICES_ARRAY=( $(echo ${JSON_SERVICES} | jq -r '.[].name') )
CLUSTERS_ARRAY=( $(echo ${JSON_CLUSTERS} | jq -r '.[].name') )

update_all

echo "Update ${ENV_ID} finished."

#!/usr/bin/env bash
set -eo pipefail

export API_ENDPOINT="api.cloud-preprod.yandex.net:443"
####### VARIABLES FROM USER
#export YAV_TOKEN=${YAV_TOKEN:?Yav token required}
SERVICE="$1"

FOLDER_ID=aoe888pej579nq07j2nb # iot
case $SERVICE in
devices)
  export IMAGE_NAME=devices-17809-fd4f0d0a55-2375
  export INSTANCE_GROUP_ID=amc2tp7tjjimp4okrs63
  export SPEC_FILE="devices-spec.yaml"
  ;;
devices-staging)
  export IMAGE_NAME=devices-14312-26d74f8a83-18621
  export INSTANCE_GROUP_ID=amc2puefk830uuvo47ju
  export SPEC_FILE="devices-staging.yaml"
  ;;
mqtt)
  export IMAGE_NAME=mqtt-v10109-f5348d9-12917
  export INSTANCE_GROUP_ID=amcbh8r1qb4pf56me7nh
  export SPEC_FILE="mqtt-spec.yaml"
  ;;
subscriptions)
  export IMAGE_NAME=subs-v10109-f5348d9-1047
  export INSTANCE_GROUP_ID=amc5macq7gn5reto5g4d
  export SPEC_FILE="subscriptions-spec.yaml"
  ;;
events)
  export IMAGE_NAME=events-v10109-f5348d9-24787
  export INSTANCE_GROUP_ID=amc2fds1fkklnfmeu43a
  export SPEC_FILE="events-spec.yaml"
  ;;
migrator)
  export IMAGE_NAME=migrator-12141-ea915e217-18633
  export INSTANCE_GROUP_ID=amcclltjkhi2r6rgrphj
  export SPEC_FILE="migrator-spec.yaml"
  ;;
tank)
  export IMAGE_NAME=tank-0e51c6e0b-28621
  export INSTANCE_GROUP_ID=amcaest8u7p3kpb3hnmm
  export SPEC_FILE="tank-spec.yaml"
  ;;
*)
  echo "Unknown service $SERVICE" 1>&2
  exit 1
  ;;
esac

IMAGE_ID=$(yc compute image get ${IMAGE_NAME} --folder-id ${FOLDER_ID} --format json --endpoint ${API_ENDPOINT} | jq -r .id)
export IMAGE_ID

####### LOAD SECRETS

####### RENDER SPEC
TMP_DIR=`mktemp -d`
trap "rm -r ${TMP_DIR}" EXIT

SPEC_RENDERED="${TMP_DIR}/spec_rendered.yaml"
envsubst '${IMAGE_ID}' < "$SPEC_FILE" > "$SPEC_RENDERED";

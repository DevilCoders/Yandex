#!/usr/bin/env bash
set -eo pipefail

export API_ENDPOINT="api.cloud.yandex.net:443"
####### VARIABLES FROM USER
#export YAV_TOKEN=${YAV_TOKEN:?Yav token required}
SERVICE="$1"

FOLDER_ID=b1gq57e4mu8e8nhimac0 # iot
case $SERVICE in
devices)
  export IMAGE_NAME=devices-17809-fd4f0d0a55-27000
  export INSTANCE_GROUP_ID=cl1p9ni5sb1dee81busc
  export SPEC_FILE="devices-spec.yaml"
  ;;
mqtt)
  export IMAGE_NAME=mqtt-v10109-f5348d9-991
  export INSTANCE_GROUP_ID=cl1olkpg19kq756kamv0
  export SPEC_FILE="mqtt-spec.yaml"
  ;;
subscriptions)
  export IMAGE_NAME=subs-v10109-f5348d9-18826
  export INSTANCE_GROUP_ID=cl1q0ii349avvfo9qhvd
  export SPEC_FILE="subscriptions-spec.yaml"
  ;;
events)
  export IMAGE_NAME=events-v10109-f5348d9-1510
  export INSTANCE_GROUP_ID=cl141cu8mrug29mt0kvi
  export SPEC_FILE="events-spec.yaml"
  ;;
migrator)
  export IMAGE_NAME=migrator-12141-ea915e217-23241
  export INSTANCE_GROUP_ID=cl1di72kb903u898t1lp
  export SPEC_FILE="migrator-spec.yaml"
  ;;
tank)
  export IMAGE_NAME=tank-0e51c6e0b-20061
  export INSTANCE_GROUP_ID=cl1lgfe85qmukanmgkks
  export SPEC_FILE="tank-spec.yaml"
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

#!/usr/bin/env bash
set -eo pipefail

export API_ENDPOINT="api.cloud-preprod.yandex.net:443"
####### VARIABLES FROM USER
#export YAV_TOKEN=${YAV_TOKEN:?Yav token required}
SERVICE="$1"

FOLDER_ID=aoe888pej579nq07j2nb # iot
case $SERVICE in
devices)
  export IMAGE_NAME=devices-23122-357db59a63-a556bc53--2021-09-22t11-03-28z
  export INSTANCE_GROUP_ID=amc2tp7tjjimp4okrs63
  export SPEC_FILE="devices-spec.yaml"
  export SKM_FILE="./files/skm-devices-md.yaml"
  ;;
mqtt)
  export IMAGE_NAME=mqtt-v15543-fa047ac-a556bc53-2021-09-22t11-03-21z
  export INSTANCE_GROUP_ID=amcbh8r1qb4pf56me7nh
  export SPEC_FILE="mqtt-spec.yaml"
  export SKM_FILE="./files/skm-mqtt-md.yaml"
  ;;
subscriptions)
  export IMAGE_NAME=subs-v15543-fa047ac-a556bc53-2021-09-22t11-03-25z
  export INSTANCE_GROUP_ID=amc5macq7gn5reto5g4d
  export SPEC_FILE="subscriptions-spec.yaml"
  export SKM_FILE="./files/skm-subscriptions-md.yaml"
  ;;
events)
  export IMAGE_NAME=events-v15543-fa047ac-a556bc53-2021-09-22t11-03-32z
  export INSTANCE_GROUP_ID=amc2fds1fkklnfmeu43a
  export SPEC_FILE="events-spec.yaml"
  export SKM_FILE="./files/skm-events-md.yaml"
  ;;
migrator)
#  export IMAGE_NAME=migrator-12141-ea915e217-18633
  export INSTANCE_GROUP_ID=amcclltjkhi2r6rgrphj
  export SPEC_FILE="migrator-spec.yaml"
  ;;
tank)
#  export IMAGE_NAME=tank-0e51c6e0b-28621
  export INSTANCE_GROUP_ID=amcaest8u7p3kpb3hnmm
  export SPEC_FILE="tank-spec.yaml"
  ;;
*)
  echo "Unknown service $SERVICE" 1>&2
  exit 1
  ;;
esac

IMAGE_ID=$(yc compute image get ${IMAGE_NAME} --profile=preprod --folder-id ${FOLDER_ID} --format json --endpoint ${API_ENDPOINT} | jq -r .id)
export IMAGE_ID

SKM_MD=$(perl -e '$_=join("", <>); s/\n/\\n/g; s/\"/\\"/g; s/\t/\x20\x20\x20\x20/g; print' ${SKM_FILE})
export SKM_MD

####### LOAD SECRETS

####### RENDER SPEC
TMP_DIR=`mktemp -d`
trap "rm -r ${TMP_DIR}" EXIT

SPEC_RENDERED="${TMP_DIR}/spec_rendered.yaml"
SPEC_RENDERED="spec_rendered.yaml"
envsubst '${IMAGE_ID},${SKM_MD}' < "$SPEC_FILE" > "$SPEC_RENDERED";

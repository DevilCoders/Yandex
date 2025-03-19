#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
source common.sh

####### CREATE INSTANCE-GROUP
set -x

export YC_TOKEN=$(yc iam create-token --profile preprod)
OP_ID=$(yc compute instance-group update  --id ${INSTANCE_GROUP_ID} --profile=preprod --file ${SPEC_RENDERED} --async --format=json --endpoint ${API_ENDPOINT} | jq -r .id)
set +x
while ! test -z ${OP_ID} && ! { yc operation get ${OP_ID} --profile=preprod --endpoint ${API_ENDPOINT} | grep "done: true"; }
do
    STATE=$(yc compute instance-group get --id ${INSTANCE_GROUP_ID} --profile=preprod  --format json --endpoint ${API_ENDPOINT} | jq -r ".managed_instances_state | .running_actual_count + \" --> \" +  .target_size");
    echo "Updating instance-group (" ${STATE} ")";
    sleep 1;
done

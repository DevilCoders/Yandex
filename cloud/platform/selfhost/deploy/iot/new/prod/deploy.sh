#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
source common.sh



####### CREATE INSTANCE-GROUP
set -x
OP_ID=$(yc --profile=prod compute instance-group update --id ${INSTANCE_GROUP_ID} --file ${SPEC_RENDERED} --async --format=json --endpoint ${API_ENDPOINT} | jq -r .id)
set +x

while ! test -z ${OP_ID} && ! { yc --profile=prod operation get ${OP_ID} --endpoint ${API_ENDPOINT} | grep "done: true"; }
do
    STATE=$(yc --profile=prod compute instance-group get --id ${INSTANCE_GROUP_ID} --format json --endpoint ${API_ENDPOINT} | jq -r ".managed_instances_state | .running_actual_count + \" --> \" +  .target_size");
    echo "Updating instance-group (" ${STATE} ")";
    sleep 1;
done

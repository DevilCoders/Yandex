#!/usr/bin/env bash

# Exit if any of the intermediate steps fail

if [[ "${YC_TOKEN}" == "" ]]
then
  echo "YC_TOKEN not set, exit with 1" >&2
  exit 1
fi

jq --arg token "${YC_TOKEN}" '{token: $token}'

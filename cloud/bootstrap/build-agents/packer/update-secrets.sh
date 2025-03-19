#!/bin/bash

[ -z "${YC_TOKEN}" ] && echo "You must set YC_TOKEN env" && exit 1
[ -z "${YAV_TOKEN}" ] && echo "You must set YAV_TOKEN env" && exit 1

SKM_MD_FILE="./files/skm-bundle.yaml"
YC_TOKEN=${YC_TOKEN} skm encrypt-bundle --config skm.yaml --bundle ${SKM_MD_FILE}

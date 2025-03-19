#!/bin/sh

SKM_DIR=`dirname $0`/../files/skm

export IAM_TOKEN=`yc iam create-token --profile preprod`
export YAV_TOKEN=`cat ~/.ssh/yav_token`
skm encrypt-md --config $SKM_DIR/skm-encrypt.yaml --format raw > $SKM_DIR/skm.yaml

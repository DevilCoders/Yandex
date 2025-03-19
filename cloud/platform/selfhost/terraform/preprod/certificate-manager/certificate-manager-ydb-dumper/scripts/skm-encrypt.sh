#!/bin/sh

export YAV_TOKEN=`cat ~/.ssh/yav_token`

set -ex

# Determine script directory
EXEC_PATH=$(dirname $0)

# Determine temp directory
SECRETS_PATH="tmp-$(date +%s)"

# Create temp dir
mkdir ${SECRETS_PATH}

# Get service account key file from yav
ya vault get version sec-01emkgbng63e3n5k7r481dj35n -o file > $SECRETS_PATH/sa-key.json

# Encrypt secrets by skm
skm encrypt-md --sa-key-file $SECRETS_PATH/sa-key.json --config $EXEC_PATH/../files/skm/skm-encrypt.yaml --format raw > $EXEC_PATH/../files/skm/skm.yaml

# Delete temp dir
rm -rf $SECRETS_PATH

#!/bin/bash

export ROOT_DIR=`dirname $0`
export KMS_HOST_NAME=$1

exec $ROOT_DIR/../scripts/install_root_config_to_host_common.sh

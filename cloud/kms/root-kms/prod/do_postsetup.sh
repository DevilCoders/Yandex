#!/bin/bash

set -ex

export KMS_HOST_NAME=$1
#export CERTIFICATE_SEC=sec-01dmjhgqzyvdr48jb19t7bpbpt
export CERTIFICATE_SEC=sec-01f336dak7ph61z54hypsdk5fw
export TVM_SEC=sec-01dq7mgdtspb6q82pr1jbmvw17

exec `dirname $0`/../scripts/do_postsetup_common.sh

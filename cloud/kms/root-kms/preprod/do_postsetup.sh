#!/bin/bash

set -ex

export KMS_HOST_NAME=$1
#export CERTIFICATE_SEC=sec-01dmjhgnxax0hksxm8t7x4f2pj
export CERTIFICATE_SEC=sec-01f336p78y7qt2peyvpqnctwt7
export TVM_SEC=sec-01e0qrw01xkyf0spjevr53c024

exec `dirname $0`/../scripts/do_postsetup_common.sh

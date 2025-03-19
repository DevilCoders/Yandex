#!/bin/bash

export KMS_HOST_NAME=$1
export CERTIFICATE_SEC=sec-01f336p78y7qt2peyvpqnctwt7

exec `dirname $0`/../scripts/install_certificate_common.sh

#!/bin/bash

export KMS_HOST_NAME=$1
export CERTIFICATE_SEC=sec-01f336dak7ph61z54hypsdk5fw

exec `dirname $0`/../scripts/install_certificate_common.sh

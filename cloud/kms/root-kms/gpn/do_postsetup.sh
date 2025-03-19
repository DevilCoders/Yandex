#!/bin/bash

set -ex

export KMS_HOST_NAME=$1
export CERTIFICATE_SEC=sec-01ecf317t5frzyyvtnmy1spw7n
export PUSH_CLIENT_SEC=sec-01dbk6f0m1ev93vqkfd8d2ztnc

`dirname $0`/../scripts/do_postsetup_common.sh

# GPN requires some additional fixups.
`dirname $0`/do_gpn_postsetup.sh

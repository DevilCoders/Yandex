#!/bin/bash

set -ex

export KMS_HOST_NAME=$1

exec `dirname $0`/../scripts/install_osquery.sh

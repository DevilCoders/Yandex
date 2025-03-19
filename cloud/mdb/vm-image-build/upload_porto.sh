#!/bin/bash

set -x

fail() {
    echo ""
    echo "FAILED $1"
    exit 1
}

cancel() {
    fail "CTRL-C detected"
}

trap cancel INT

if [ $# -lt 2 ]
then
    echo "usage: $0 <image path> <dbname>" 1>&2
    exit 1
fi

IMAGE_PATH=$1
DBNAME=$2
BUCKET_TEST="dbaas-images-vm-built-test"

IMAGE_S3_TEST="s3://${BUCKET_TEST}/${DBNAME}/porto-template-$(date +%F).gz"

/usr/bin/s3cmd -c /etc/s3cmd.cfg put ${IMAGE_PATH} ${IMAGE_S3_TEST}

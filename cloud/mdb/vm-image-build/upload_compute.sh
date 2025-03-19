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

if [ $# -lt 1 ]
then
    echo "usage: $0 <image name>" 1>&2
    exit 1
fi

IMAGE_NAME="$1"
SNAPSHOT_NAME="dbaas-$IMAGE_NAME-image-$(date +%F)-$(date +%s)"
BUCKET="a2bd1a42-69fd-451a-8dee-47ba04fbacbf"
S3_IMAGE_NAME="${SNAPSHOT_NAME}.img"

s3cmd -c /etc/ext-s3cmd.cfg sync "${IMAGE_NAME}.img" "s3://$BUCKET/$S3_IMAGE_NAME" || fail "Unable to upload to s3"

exit 0

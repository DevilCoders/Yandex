#!/bin/sh

set -ex

export LOCKBOX_ID=e6q6ichev79ci87tafdb
export LOCKBOX_KEY=kms-israel-s3-terraform-sa-key
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

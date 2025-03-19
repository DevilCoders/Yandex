#!/bin/sh

set -ex

export LOCKBOX_ID=e6q6vmqnuipkdtokhrvh
export LOCKBOX_KEY=sa-terraform-s3-secret
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

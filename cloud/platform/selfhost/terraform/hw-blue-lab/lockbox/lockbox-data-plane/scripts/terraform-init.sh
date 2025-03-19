#!/bin/sh

set -ex

export LOCKBOX_ID=e6qmciihodn3iq7up5kb
export LOCKBOX_KEY=sa-terraform-s3-secret
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

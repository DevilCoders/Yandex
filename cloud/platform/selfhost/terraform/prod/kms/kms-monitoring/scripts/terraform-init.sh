#!/bin/sh

set -ex

export SECRET_KEY=sec-01dz42hzha0gewq57fykbngcx4
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

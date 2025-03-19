#!/bin/sh

set -ex

export SECRET_KEY=sec-01e8pt0er87x80hcd7rkp7j2f0
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

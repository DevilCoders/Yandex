#!/bin/sh

set -ex

export SECRET_KEY=sec-01e25y2g51qnq168z0h77f0nwa
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

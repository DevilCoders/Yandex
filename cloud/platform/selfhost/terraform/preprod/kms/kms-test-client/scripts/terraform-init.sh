#!/bin/sh

set -ex

export SECRET_KEY=sec-01dhw139y8smwa9qdc2j2yh1yf
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

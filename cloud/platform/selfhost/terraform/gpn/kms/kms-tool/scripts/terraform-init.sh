#!/bin/sh

set -ex

export SECRET_KEY=sec-01ecb8bz9jg9cc2kbyswd5mpj1
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

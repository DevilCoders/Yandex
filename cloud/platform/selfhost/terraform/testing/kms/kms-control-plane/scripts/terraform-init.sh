#!/bin/sh

set -ex

export SECRET_KEY=sec-01eax1xwmqn2xqwycdqdffet81
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

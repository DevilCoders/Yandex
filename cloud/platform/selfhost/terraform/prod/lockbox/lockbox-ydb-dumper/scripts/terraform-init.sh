#!/bin/sh

set -ex

export SECRET_KEY=sec-01eskq8zzz0kjhhjz0zkrpftbd
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

#!/bin/sh

set -ex

export SECRET_KEY=sec-01eetd95sb15qw9v0ygek6tj0q
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

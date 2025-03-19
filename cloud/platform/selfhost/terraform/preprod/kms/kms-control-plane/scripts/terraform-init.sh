#!/bin/sh

set -ex

export SECRET_KEY=sec-01dbmrxbqc9xba5e6m102jxfza
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

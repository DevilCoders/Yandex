#!/bin/sh

set -ex

export SECRET_KEY=sec-01f247ekh7kad4jjptgwedyt46
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

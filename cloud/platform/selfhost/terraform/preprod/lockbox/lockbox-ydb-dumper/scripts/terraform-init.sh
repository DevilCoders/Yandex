#!/bin/sh

set -ex

export SECRET_KEY=sec-01esh0wk0q181kag1phdg77g0s
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

#!/bin/sh

set -ex

export SECRET_KEY=sec-01e1y1j0snwpjtjwvz2kqdq2r3
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

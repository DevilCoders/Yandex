#!/bin/sh

set -ex

export SECRET_KEY=sec-01ewg9abb9cxgfg6pgy6v93wv1
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

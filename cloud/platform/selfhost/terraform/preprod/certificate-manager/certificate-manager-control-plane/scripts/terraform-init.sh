#!/bin/sh

set -ex

export SECRET_KEY=sec-01dvncym5jp9y9115d6bh5f36e
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

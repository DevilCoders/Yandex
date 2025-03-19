#!/bin/sh

set -ex

export SECRET_KEY=sec-01e58j3vepqxcd98msv6qsbaee
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

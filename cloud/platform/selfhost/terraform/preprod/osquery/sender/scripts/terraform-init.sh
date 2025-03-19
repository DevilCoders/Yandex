#!/bin/sh

set -ex

export SECRET_KEY=sec-01er2btfwv57twm8m01remjge6
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh

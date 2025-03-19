#!/bin/sh

set -ex

SECRET_KEY=$(yc --profile preprod lockbox payload get --id fc310n0a5stunac2vd22 --key secret)
cd `dirname $0`/..
terraform init $* -backend-config="secret_key=$SECRET_KEY"

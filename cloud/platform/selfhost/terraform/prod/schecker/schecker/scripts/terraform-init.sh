#!/bin/sh

set -ex

SECRET_KEY=$(yc --profile prod lockbox payload get --id e6qectt4bg15nb2256me --key secret)
cd `dirname $0`/..
terraform init $* -backend-config="secret_key=$SECRET_KEY"

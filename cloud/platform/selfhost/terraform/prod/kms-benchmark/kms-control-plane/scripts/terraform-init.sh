#!/bin/sh

set -ex

cd `dirname $0`/..
terraform init $* -backend-config="secret_key=`yc lockbox payload get e6qcabuvld74rusms5fv --key secret_key --profile prod`"

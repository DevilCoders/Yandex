#!/bin/sh

set -ex

#export SECRET_KEY=
export LOCKBOX_ID=e6q2caquh6pub24250cs
export LOCKBOX_KEY=secret

if [ "$SECRET_KEY" != "" ]; then
  SECRET=`ya vault get version $SECRET_KEY -o secret_key`
else
  SECRET=`yc lockbox payload get --id $LOCKBOX_ID --key $LOCKBOX_KEY --profile prod`
fi

terraform init $* -backend-config="secret_key=$SECRET"

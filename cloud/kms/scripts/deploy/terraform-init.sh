#!/bin/bash

if [ "$SECRET_KEY" != "" ]; then
  SECRET=`ya vault get version $SECRET_KEY -o secret_key`
else
  SECRET=`yc lockbox payload get --id $LOCKBOX_ID --key $LOCKBOX_KEY --profile prod-fed-user`
fi

terraform init $* -backend-config="secret_key=$SECRET"

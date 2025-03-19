#!/bin/bash
ENV=$1
SECRET=sec-01ewn2b4vf3vmj18c65w1b8wh6

if [[ -n $ENV ]]; then
  cd "$ENV"
fi

terraform init -backend-config="secret_key=$(ya vault get version "${SECRET}" -o AccessSecretKey)"

#!/bin/bash

set -e

YC_PROFILE="connman-$PROFILE"
YC_TOKEN=$(yc iam create-token --profile="$YC_PROFILE")
export YC_TOKEN

TF_VAR_DB_ADMIN_PASSWORD=$(ya vault get version "$DB_ADMIN_PASSWORD_SECRET" -o cm-admin)
export TF_VAR_DB_ADMIN_PASSWORD

terraform workspace select "$PROFILE"
terraform "$@"

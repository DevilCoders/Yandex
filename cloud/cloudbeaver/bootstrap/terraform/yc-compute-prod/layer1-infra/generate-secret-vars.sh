#!/usr/bin/env bash

set -e

PROD_SECRET_VERSION="ver-01fptse115phjtca8q7hk800t9"

if [[ -z "${PROD_SECRET_VERSION}" ]];
then
    echo "Error: prod secret version is required"
    exit 1
fi


static_access_key_data=$(ya vault get version ${PROD_SECRET_VERSION} -o sa-static-key.json)

s3_admin_access_key=$(echo ${static_access_key_data}|jq -r "values.access_key.key_id")
s3_admin_secret_key=$(echo ${static_access_key_data}|jq -r "values.secret")

cat << EOF > secrets.auto.tfvars
s3_admin_access_key = "${s3_admin_access_key}"
s3_admin_secret_key = "${s3_admin_secret_key}"
EOF

ya vault get version ${PROD_SECRET_VERSION} -o sa-key.json > sa.json

cat << EOF > .envrc
export AWS_ACCESS_KEY_ID=${s3_admin_access_key}
export AWS_SECRET_ACCESS_KEY=${s3_admin_secret_key}
EOF

# gpg importing
# password: ya vault get version ${PROD_SECRET_VERSION} -o cloudbeaver-terraform-prod-gpg-password
ya vault get version ${PROD_SECRET_VERSION} -o cloudbeaver-terraform-prod-gpg-private.key | gpg --import --

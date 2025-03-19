#!/usr/bin/env bash

set -e

S3_ADMIN_SECRET_VERSION="$1"
MDB_ADMIN_SECRET_VERSION="$2"
SA_SECRET_VERSION="$3"

if [[ -z "$S3_ADMIN_SECRET_VERSION" ]];
then
    echo "require s3-admin secret version"
fi

s3_admin_access_key=$(ya vault get version "$S3_ADMIN_SECRET_VERSION" -o access_key)
s3_admin_secret_key=$(ya vault get version "$S3_ADMIN_SECRET_VERSION" -o secret_key)

cat << EOF > secrets.auto.tfvars
s3_admin_access_key = "$s3_admin_access_key"
s3_admin_secret_key = "$s3_admin_secret_key"
EOF

mdb_admin_token=""
if [[ -n "$MDB_ADMIN_SECRET_VERSION" ]];
then
    mdb_admin_token=$(ya vault get version "$MDB_ADMIN_SECRET_VERSION" -o token)
    cat << EOF >> secrets.auto.tfvars
mdb_admin_token     = "$mdb_admin_token"
EOF
fi

if [[ -n "$SA_SECRET_VERSION" ]];
then
    ya vault get version $SA_SECRET_VERSION -o sa.json > sa.json
fi

cat << EOF > .envrc
export AWS_ACCESS_KEY_ID=$s3_admin_access_key
export AWS_SECRET_ACCESS_KEY=$s3_admin_secret_key
EOF

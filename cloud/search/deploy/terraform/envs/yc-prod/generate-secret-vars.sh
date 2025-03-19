#!/usr/bin/env bash

set -e

S3_ADMIN_SECRET_VERSION="ver-01ewtzt3davx3ntvnq02d60hyp"
SA_SECRET_VERSION="ver-01ex4jknr886k8z37qnbtpbz46"

s3_admin_access_key=$(ya vault get version "$S3_ADMIN_SECRET_VERSION" -o access_key)
s3_admin_secret_key=$(ya vault get version "$S3_ADMIN_SECRET_VERSION" -o secret_key)

cat << EOF > secrets.auto.tfvars
s3_admin_access_key = "$s3_admin_access_key"
s3_admin_secret_key = "$s3_admin_secret_key"
EOF

ya vault get version $SA_SECRET_VERSION -o sa.json > sa.json

cat << EOF > .envrc
export AWS_ACCESS_KEY_ID=$s3_admin_access_key
export AWS_SECRET_ACCESS_KEY=$s3_admin_secret_key
EOF

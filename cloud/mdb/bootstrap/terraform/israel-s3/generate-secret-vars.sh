#!/usr/bin/env bash

S3_SECRET_VERSION=ver-01fvtqb12bqr6r8brn8g369p8f

cat << EOF > .envrc
export AWS_ACCESS_KEY_ID=$(ya vault get version "$S3_SECRET_VERSION" -o id)
export AWS_SECRET_ACCESS_KEY=$(ya vault get version "$S3_SECRET_VERSION" -o secret)
EOF

ya vault get version ver-01fyykecnzqappn9fy516f83y8 -o config.yaml > israel_ycp_config.yaml


#!/bin/sh

terraform init $* -backend-config="secret_key=$(ya vault get version sec-01djmfwpjn2a5hwzn7gcm2mmcz -o access-secret-key-terraform-maintainer)"

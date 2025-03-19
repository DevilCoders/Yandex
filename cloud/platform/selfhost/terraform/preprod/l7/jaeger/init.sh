#!/usr/bin/env bash
SECRET=sec-01f3867zcdmj8r2ary802hv5y9
terraform init -backend-config="secret_key=$(ya vault get version "${SECRET}" -o secret_key)" "$@"
terraform refresh

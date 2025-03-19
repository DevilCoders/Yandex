#!/usr/bin/env bash
SECRET=sec-01e4r23nq5kf8ctqqf4t3gaes5
terraform init -backend-config="secret_key=$(ya vault get version "${SECRET}" -o secret_key_prod)"

#!/usr/bin/env bash
SECRET=sec-01cwtqr1fsz56yx166hj319sqb
terraform init -backend-config="secret_key=$(ya vault get version "${SECRET}" -o secret_key)"
terraform refresh

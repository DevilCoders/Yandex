#!/usr/bin/env bash
SECRET=sec-01e4ybzghx0wbs59yhkf8f122b
terraform init -backend-config="secret_key=$(ya vault get version "${SECRET}" -o secret_key)"

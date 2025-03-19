#!/bin/sh
# Run terraform
terraform init $* -backend-config="secret_key=$(yc --profile trail-prod lockbox payload get --id e6qgh6gdu798i2r3s2ir --key sa_terraform_secret_key)"

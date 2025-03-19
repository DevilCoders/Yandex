#!/bin/sh
# Run terraform
terraform init $* -backend-config="secret_key=$(yc --profile trail-preprod lockbox payload get --id fc3aeph1lh6b3qfk0k0h --key sa_terraform_secret_key)"

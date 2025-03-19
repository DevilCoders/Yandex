#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" == "prod" ]]; then
    SEC_ID="sec-01fe15fc1zgzpp28yy435sn2cn"
elif [[ "$PROFILE" == "preprod" ]]; then
    SEC_ID="sec-01feeq75tm4nn80p8ewfgxaw7r"
else
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

TERRAFORM_DIR="$(dirname "$0")/terraform/$PROFILE"
pushd $TERRAFORM_DIR

terraform init $@ \
    -backend-config="secret_key=$(ya vault get version ${SEC_ID} -o secret_key)"

popd

#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" == "prod" ]]; then
    ACCESS_KEY="QVe8kxzT18O3BcoXjjEv"
    SEC_ID="sec-01fe15fc1zgzpp28yy435sn2cn"
elif [[ "$PROFILE" == "preprod" ]]; then
    ACCESS_KEY="RwdLNB7Hmvb6Ap_rTohD"
    SEC_ID="sec-01feeq75tm4nn80p8ewfgxaw7r"
else
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

s3cmd $@ \
    --config="$(dirname "$0")/s3/$PROFILE.cfg" \
    --access_key=$ACCESS_KEY \
    --secret_key="$(ya vault get version ${SEC_ID} -o secret_key)"

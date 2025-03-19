#!/bin/bash
set -e

ENV_ID=datacloud-aws
yav create secret ${ENV_ID}-sa-keys \
    --roles owner:abc:yc_iam_dev:administration \
    --comment "${ENV_ID} Service Account keys" \
    --tags ${ENV_ID}

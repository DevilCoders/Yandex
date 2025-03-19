#!/usr/bin/env bash

export ST_OAUTH_TOKEN_FILE=${ST_OAUTH_TOKEN_FILE:-~/.config/oauth/st}
export ST_OAUTH_URL="https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b"
export JUGGLER_OAUTH_TOKEN_FILE=${JUGGLER_OAUTH_TOKEN_FILE:-~/.config/oauth/juggler}
export JUGGLER_OAUTH_URL="https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0"
export S3_CREDS_LOCKBOX_ID=${S3_CREDS_LOCKBOX_ID:-e6qli9h4q1kovkarehkh}
export S3_AWS_PROFILE=${S3_AWS_PROFILE:-iam-metadata-release}
export S3_ENDPOINT=${S3_ENDPOINT:-https://s3.mds.yandex.net}
export S3_BUCKET=${S3_BUCKET:-iam}
export CLOUD_GO_PATH=${CLOUD_GO_PATH:-"${GOPATH:-$HOME/go}/src/bb.yandex-team.ru/cloud/cloud-go"}

export STANDS=${STANDS:-"internal-dev testing internal-prestable preprod doublecloud-aws-preprod internal-prod doublecloud-aws-prod prod israel"}

export IAM_METADATA_ENV_VARS_AVAILABLE=yes

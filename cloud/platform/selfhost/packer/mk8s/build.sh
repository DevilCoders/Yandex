#!/usr/bin/env bash

set -euxo pipefail

export COMMIT_REVISION="${COMMIT_REVISION:?Commit revision required.}"
export COMMIT_AUTHOR="$USER"
export COMMIT_MESSAGE="${DESCRIPTION:?Description required.}"

export YC_TOKEN="${YC_TOKEN:?OAUTH token required}"
export SSH_PRIVATE_KEY_FILE="${SSH_PRIVATE_KEY_FILE:?Pass path to your staff private ssh key}"

packer validate ./mk8s-controller.json
packer build ./mk8s-controller.json

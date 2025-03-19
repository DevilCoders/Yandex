#!/usr/bin/env bash

source $(dirname "$0")/../common/profile.sh
COMMAND=$(COMMAND_DEFAULT="" $(dirname "$0")/../common/command.sh $@)

export REMOTE_SELECTOR="cloud_${PROFILE}_serverless_router"
$(dirname "$0")/../common/remote.sh ${COMMAND}

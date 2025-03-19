#!/usr/bin/env bash

source $(dirname "$0")/common/profile.sh
COMMAND=$(COMMAND_DEFAULT="" $(dirname "$0")/common/command.sh $@)

REMOTE_SELECTOR="cloud_${PROFILE}_api-gateway_tf" $(dirname "$0")/common/remote.sh ${COMMAND}

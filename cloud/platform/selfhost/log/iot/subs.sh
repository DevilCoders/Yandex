#!/usr/bin/env bash

source $(dirname "$0")/../common/profile.sh
COMMAND=$(COMMAND_DEFAULT="" $(dirname "$0")/../common/command.sh $@)

if [[ -n "$LEGACY" ]]; then
  export REMOTE_SELECTOR="cloud_${PROFILE}_iot-subscriptions-${PROFILE}"
else
  export REMOTE_SELECTOR="cloud_${PROFILE}_iot-subscriptions"
fi

$(dirname "$0")/../common/remote.sh ${COMMAND}

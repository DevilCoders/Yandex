#!/usr/bin/env bash

source $(dirname "$0")/../common/profile.sh
COMMAND=$(COMMAND_DEFAULT="" $(dirname "$0")/../common/command.sh $@)

export REMOTE_SELECTOR="cloud_${PROFILE}_ycf_packer"
$(dirname "$0")/../common/remote.sh ${COMMAND}

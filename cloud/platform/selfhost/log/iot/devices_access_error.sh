#!/usr/bin/env bash

source $(dirname "$0")/../common/log.sh
COMMAND=$(COMMAND_DEFAULT_TAIL_N=10 $(dirname "$0")/../common/command.sh $@)

$(dirname "$0")/devices_access.sh "${GRPC_ERROR} | ${COMMAND}"

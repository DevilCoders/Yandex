#!/usr/bin/env bash

source $(dirname "$0")/../common/log.sh
source $(dirname "$0")/common_log.sh
COMMAND=$(COMMAND_DEFAULT_TAIL_N=10 $(dirname "$0")/../common/command.sh $@)

$(dirname "$0")/mqtt_access.sh "${IOT_ERROR} | ${COMMAND}"

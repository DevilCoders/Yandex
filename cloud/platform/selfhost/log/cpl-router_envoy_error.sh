#!/usr/bin/env bash

DAY=$($(dirname "$0")/common/day.sh)
source $(dirname "$0")/common/log.sh
COMMAND=$($(dirname "$0")/common/command.sh $@)

$(dirname "$0")/cpl-router.sh "cat /var/log/fluent/error_log.envoy.${DAY//-/}.log | ${COMMAND}"

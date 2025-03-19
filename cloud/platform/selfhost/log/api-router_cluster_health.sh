#!/usr/bin/env bash

COMMAND=$(COMMAND_DEFAULT="grep failed" $(dirname "$0")/common/command.sh $@)

$(dirname "$0")/api-router.sh "curl -k http://localhost:9901/clusters 2>/dev/null | sed \"s/^/\$(hostname): /\" | grep health_flags | ${COMMAND}"

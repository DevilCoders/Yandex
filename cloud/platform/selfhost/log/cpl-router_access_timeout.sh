#!/usr/bin/env bash

COMMAND=$($(dirname "$0")/common/command.sh $@)

$(dirname "$0")/cpl-router_access.sh "grep \"\\\"status\\\":0\" | ${COMMAND}"

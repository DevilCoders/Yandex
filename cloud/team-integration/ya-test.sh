#!/usr/bin/env bash
cd "$(dirname "$0")" || exit $?
source ya.env

PROJECT_NAME=$(basename "$PWD")
PREFIX="test(${PROJECT_NAME?}):"

ACTION_NAME='ya test'
ACTION_START=$(date +%s);echo -e "${PREFIX?} starting ${ACTION_NAME?s}"
ya make \
    ${YA_BUILD_OPTS?} \
    || exit $?
echo -e "${PREFIX?} finished ${ACTION_NAME?} in $(($(date +%s) - ${ACTION_START?})) seconds"

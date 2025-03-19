#!/usr/bin/env bash
cd "$(dirname "$0")" || exit $?
source ya.env

PROJECT_NAME=$(basename "$PWD")
PREFIX="package(${PROJECT_NAME?}):"

ACTION_NAME='ya package'
ACTION_START=$(date +%s);echo -e "${PREFIX?} starting ${ACTION_NAME?s}"
ya package \
    ${YA_CHECKOUT_OPTS?} \
    --debian --not-sign-debian \
    team-integration-package/ya-package.json \
    || exit $?
rm -v packages.json
echo -e "${PREFIX?} finished ${ACTION_NAME?} in $(($(date +%s) - ${ACTION_START?})) seconds"

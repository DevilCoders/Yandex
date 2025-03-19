#!/usr/bin/env bash
cd "$(dirname "$0")" || exit $?
source ya.env

PROJECT_NAME=$(basename "$PWD")
PROJECT_ROOT=${1:-"${HOME}/IdeaProjects/${PROJECT_NAME?}"}
PREFIX="idea(${PROJECT_NAME?}):"

ACTION_NAME='ya checkout'
ACTION_START=$(date +%s);echo -e "${PREFIX?} starting ${ACTION_NAME?s}"
ya make \
    ${YA_BUILD_OPTS?} \
    -j0 \
    || exit $?
echo -e "${PREFIX?} finished ${ACTION_NAME?} in $(($(date +%s) - ${ACTION_START?})) seconds"

ACTION_NAME='ya idea'
ACTION_START=$(date +%s);echo -e "${PREFIX?} starting ${ACTION_NAME?s}"
ya ide idea \
    --project-name="${PROJECT_NAME?}" \
    --project-root="${PROJECT_ROOT?}" \
    --iml-in-project-root \
    --with-content-root-modules \
    --with-long-library-names \
    --local \
    --ascetic \
    || exit $?
echo -e "${PREFIX?} finished ${ACTION_NAME?} in $(($(date +%s) - ${ACTION_START?})) seconds"

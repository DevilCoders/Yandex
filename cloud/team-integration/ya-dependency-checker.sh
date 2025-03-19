#!/usr/bin/env bash
cd "$(dirname "$0")" || exit $?
source ya.env

TARGET="team-integration-application/src/test"

ya java \
    dependency-tree \
    ${TARGET?} \
    | grep -v "cloud/team-integration" \
    | grep -v -E '\(\*\)$' \
    | sed -e "s/|   //g" \
    | sed -e "s/|-->//" \
    | sort \
    | uniq

#!/bin/bash
export SCRIPTS_PATH="${1:-.}"
export PROFILE="${YDB_PROFILE:-default}"

find "$SCRIPTS_PATH" -type f -iname \*.sql|xargs -L1 -r ydb -p $PROFILE scripting yql -f

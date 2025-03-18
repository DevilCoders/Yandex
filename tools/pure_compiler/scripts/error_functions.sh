#!/bin/sh
# Just common function definitions, does nothing.
alias warn="echo 1>&2" 

alias CheckResult="DoCheckResult \$?"

DoCheckResult() {
    local RESULT CALL
    RESULT=$1
    shift
    CALL="${@:-Last call}"
    if [ $RESULT -ne 0 ]
    then
        warn "Error: $CALL returned $RESULT."
        exit 1
    fi
}

RunAndKill() {
    local RESULT
    "$@"
    RESULT=$?
    if [ $RESULT -ne 0 ]
    then
        warn "Error: $@ returned $RESULT."
        kill $$
        exit 1
    fi
}

RunAndCheck() {
    RunAndKill "$@"
}

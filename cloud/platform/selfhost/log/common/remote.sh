#!/bin/bash
#set -x #echo on

if [[ -z "$REMOTE_SELECTOR" ]]; then
    echo "REMOTE_SELECTOR is undefined!"
    exit 1
fi

if [[ ! "$SILENT" == "true" ]]; then
    (>&2 echo "Execute on ${REMOTE_SELECTOR}")
fi

if [[ -z "$TIMEOUT" ]]; then
    TIMEOUT="88"
fi

if (( $# > 0 )); then
    REMOTE_COMMAND="$@"
    if [[ ! "$SILENT" == "true" ]]; then
        (>&2 echo "REMOTE_COMMAND: ${REMOTE_COMMAND}")
    fi
    if [[ -z "$LOCAL_COMMAND" ]]; then
        LOCAL_COMMAND="cat"
    else
        if [[ ! "$SILENT" == "true" ]]; then
            (>&2 echo "LOCAL_COMMAND: ${LOCAL_COMMAND}")
        fi
    fi
else
    REMOTE_COMMAND="echo \"{\\\"hostname\\\":\\\"\$(hostname | sed 's/\s//g')\\\",\\\"fqdn\\\":\\\"\$(hostname -A | sed 's/\s//g')\\\",\\\"ipv4\\\":\\\"\$(hostname -I | sed 's/^.*\(172[^ ]\+\).*$/\1/' | sed 's/\s//g')\\\",\\\"ipv6\\\":\\\"\$(hostname -I | sed 's/^.*\(2a02[^ ]\+\).*$/\1/')\\\"}\""
    LOCAL_COMMAND="grep \"^{\" | jq -s \". | sort_by(.host)\""
fi

if [[ "$PSSH" == "false" ]]; then
    if [[ -z "$DEBUG" ]]; then
        EXECUTER_QUIET=" --quiet"
        EXECUTER_DEBUG=" 2>/dev/null"
    else
        EXECUTER_QUIET=""
        EXECUTER_DEBUG=""
    fi

    executer${EXECUTER_QUIET} --user ${USER} --shmux-timeout ${TIMEOUT} p_exec %${REMOTE_SELECTOR} ${REMOTE_COMMAND}${SDEBUG} | grep ${USER} | sed 's/^[^:]*: //' | eval ${LOCAL_COMMAND}
else
    if [[ -z "$THREADS" ]]; then
        THREADS="8"
    fi
    if [[ "$NO_YUBIKEY" == "true" ]]; then
      YUBIKEY_OPTION="--no-yubikey"
    fi
    pssh run --ignore-failed --conn-timeout ${TIMEOUT} -p ${THREADS} ${YUBIKEY_OPTION} "${REMOTE_COMMAND}" C@${REMOTE_SELECTOR} | eval ${LOCAL_COMMAND}
fi

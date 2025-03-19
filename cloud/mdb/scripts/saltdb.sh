#!/usr/bin/env bash

[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

DBNAME=${1:-dbm}

if [[ -n "$ECHO" ]] && [[ "$ECHO" != "0" ]]; then
    PSQL_OPTS+=" -e"
fi

hosts=(saltdb01e.mail.yandex.net saltdb01i.mail.yandex.net saltdb01h.mail.yandex.net)
if [[ -z "$MASTER" ]] || [[ "$MASTER" == "0" ]]; then
    host=${hosts[1]}
else
    for h in ${hosts[@]}; do
        if [[ `ssh $h sudo -u postgres ~postgres/.role.sh </dev/null 2>/dev/null` == 'M' ]]; then
            host=$h
        fi
    done
    [ -z "$host" ] && echo "Master not found" >&2 && exit 1
fi

if [ -t 0 ]; then
    ssh -t $host psql $DBNAME
else
    ssh $host sudo -u postgres "psql ${DBNAME} ${PSQL_OPTS} -f -" <&0
    #ssh $SALTDB "cd /tmp && sudo -u postgres psql ${DBNAME} ${PSQL_OPTS} -c '$(cat)'"
fi

#!/usr/bin/env bash

WD_LOCKFILE=/var/tmp/yasmagent-mdb-wd.lock

YASM_PIDFILE=/usr/local/yasmagent/run/agent.pid
YASM_PORT=11003

exec 9> $WD_LOCKFILE || exit
flock --wait=15 --exclusive 9 || exit

yasm_listen_its_port() {
    curl -s "http://localhost:$YASM_PORT/json/" > /dev/null 2>&1
    return $?
}

yasm_pid_points_to_running_process() {
    if [[ -s $YASM_PIDFILE ]]; then
        kill -0 "$(cat $YASM_PIDFILE)"
        return $?
    fi
    return 0
}

kill_old_yasm_that_use_port() {
    # MDB-3430 - case
    if [[ -s $YASM_PIDFILE ]]; then
        file_child_pid=$(pgrep --parent "$(cat $YASM_PIDFILE)")
        used_pid=$(netstat -tpln | grep $YASM_PORT | awk '{print $NF}' | awk -F'/' '{print $1}')

        if [[ -n "$used_pid" && "$used_pid" != "$file_child_pid" ]] ; then
            service yasmagent status > /dev/null 2>&1 || (kill "$used_pid" && service yasmagent restart)
        fi
    fi
}

yasm_listen_its_port || (service yasmagent restart; sleep 10)
yasm_pid_points_to_running_process || (service yasmagent restart; sleep 10)
kill_old_yasm_that_use_port


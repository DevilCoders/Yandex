#!/usr/bin/env bash

PATTERN="\/sbin\/syslog-ng"
RESTART_CMD="/etc/init.d/syslog-ng restart"
SLEEP_DELAY="10"
LOG="/var/log/syslog-ng-watchdog/syslog-ng-watchdog.log"

check_syslog() {
        for pid in $(pgrep -f ${PATTERN}); do
            if [[ "$(readlink /proc/1/ns/pid)" == "$(readlink /proc/$pid/ns/pid)" ]]; then
                if /etc/init.d/syslog-ng status 2>&1 >> /dev/null; then
                    return 0
                fi
            fi
        done
        return 1
}

cleanup_zombies() {
        cmds=$(ps axo pid,state,cmd | awk '/\S*Z\S* \[syslog-ng\] <defunct>$/ { print $1 }')
        for cmd in $cmds; do
            if [[ "$(readlink /proc/1/ns/pid)" != "$(readlink /proc/$cmd/ns/pid)" ]]; then
                continue
            fi
            parent=$(ps -o ppid= -p $cmd | xargs)
            echo $cmd | xargs -r kill -15
            echo $parent | xargs -r kill -15
            [[ -d /proc/$cmd ]]    && echo $cmd | xargs -r kill -9
            [[ -d /proc/$parent ]] && echo $parent | xargs -r kill -9
            [[ ! -d /proc/$cmd ]]  && echo "Reaped zombie $cmd" >> $LOG
        done
}

if ! check_syslog ; then
        sleep $SLEEP_DELAY
        if ! check_syslog ; then
                echo $(date) >> $LOG
                cmds=$(ps ax | awk '/syslog-ng/ && !/watchdog/ {print $1}')
                for cmd in $cmds; do
                    if [[ "$(readlink /proc/1/ns/pid)" != "$(readlink /proc/$cmd/ns/pid)" ]]; then
                        continue
                    fi
                    echo $cmd | xargs -r kill -15
                    [[ ! -d /proc/$cmd ]] && echo $cmd | xargs -r kill -9
                done
                $RESTART_CMD 2>&1 >> $LOG
        fi
fi

cleanup_zombies


# EOF


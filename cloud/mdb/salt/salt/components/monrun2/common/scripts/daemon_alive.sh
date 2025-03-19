#!/bin/bash

me=${0##*/}
me=${me%.*}

USE_SUDO=0

while getopts "sr:m:d:p:" OPTION
do
    case $OPTION in
        s)
            USE_SUDO="1"
        ;;
        m)
            MAX_INSTANCES="$OPTARG"
        ;;
        d)
            DAEMON="$OPTARG"
        ;;
        p)
            PIDFILE="$OPTARG"
        ;;
    esac
done

# default values
max_instances=${MAX_INSTANCES:-1}
daemon=${DAEMON:-$me}
pidfile=${PIDFILE:-/var/run/"$daemon".pid}

die () {
    printf "%s;%s\n" "$1" "$2"
    exit 0
}

check_proc_linux()
{
    # 0. defaults
    local current_instances process pid
    process=$1
    current_instances=0
    pid=''
    # 1. check by pidfile.
    if [ -s "${pidfile}" ]; then
        # read pid
        if [ "$USE_SUDO" = "1" ]
        then
            pid=$(sudo cat "${pidfile}")
        else
            pid=$(cat "${pidfile}")
        fi
        # check that pid
        ps --pid "${pid}" &>/dev/null || die 2 "${process}: stale pidfile present"
    else
        # No pidfile!
        die 2 "${process}: no pidfile found"
    fi
    
    # 2. Count instances, taking LXC namespace separation into consideration
    # pgrep doesnt know any difference between, e.g. CROND and crond, and -f is too risky.
    if [ -e '/proc/self/cpuset' ]; then 
        own_domain=$(cat /proc/self/cpuset) # e.g.: /
        for proc_pid in $(pgrep -P 1 -f "^${process}")
        do
            proc_domain=$(cat /proc/"$proc_pid"/cpuset 2>/dev/null)
            [ "x${own_domain}" = x"${proc_domain}" ] && current_instances=$((current_instances+1))
        done
    else
        current_instances=$(pgrep -P 1 -f "^${process}" | wc -l)
    fi
    [ "${current_instances}" -gt "${max_instances}" ] && die 1 "${process}: abnormal process count: ${current_instances} (max ${max_instances})"
    [ "${current_instances}" -eq 0 ] && die 2 "${process}: not running"
    die 0 "OK"
}

case $(uname) in
    Linux)
        check_proc_linux "${daemon}"
    ;;
    *)
        die 1 "${process}: OS $(uname) is not supported"
    ;;
esac

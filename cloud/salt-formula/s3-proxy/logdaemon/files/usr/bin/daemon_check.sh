#!/bin/sh

DAEMON=$1
SCRIPT_NAME=$(basename "$0");

check_daemon_regular(){
    daemon_regular=$(ps -e -o state,cmd | grep -Ev '^Z|awk|grep|monrun' | grep -v "$SCRIPT_NAME" | grep -c "$DAEMON");
    if [ "$daemon_regular" -gt 0 ];
    then
        echo "0;OK";
    else
        echo "2;$DAEMON daemon is not alive";
    fi
}

check_daemon_dom0(){
    daemon_dom0=$(for n in $(ps waux | grep -Ev 'grep|daemon_check|monrun' |grep "$DAEMON" 2>/dev/null |awk '{print $2}');
            do
                cat /proc/$n/status 2>/dev/null |grep -v grep| grep -Ei "envID:" |awk '{print $2}' | grep -E ^0$
            done |uniq |wc -l
             );
    if [ "$daemon_dom0" -gt 0 ];
    then
        echo "0;OK";
    else
        echo "2;$DAEMON daemon is not alive";
    fi
}

check_daemon_dom0_lxc(){
    daemon_dom0=$(for n in $(ps waux | grep -Ev 'grep|daemon_check|monrun' | grep "$DAEMON" 2>/dev/null |awk '{print $2}');
            do
                if
                [ x"$(cat /proc/"$n"/cpuset 2>/dev/null)" = "x/" ]; then echo dom0_host;
                # CADMIN-6316: sometimes kernel does not init cpuset
                elif
                [ x"$(cat /proc/"$n"/cgroup 2>/dev/null | awk -F: '/cpuset/ { print $NF}')" = "x/" ]; then echo dom0_host;
                else
                echo virtual_host;
                fi
            done | grep -c dom0_host
             );
    if [ "$daemon_dom0" -gt 0 ];
    then
        echo "0;OK";
    else
        echo "2;$DAEMON daemon is not alive";
    fi
}

. /usr/local/sbin/autodetect_environment

if   [ "$is_dom0_host" -eq 1 ] && [ "$is_openvz_host" -eq 1 ]; then
check_daemon_dom0 "$DAEMON"

elif [ "$is_dom0_host" -eq 1 ] && [ "$is_lxc_host" -eq 1 ]; then
check_daemon_dom0_lxc "$DAEMON"

elif [ "$is_virtual_host" -eq 1 ] || [ "$is_classic_host" -eq 1 ]; then
check_daemon_regular "$DAEMON"

else echo "2;Unable to detect host type"

fi

#!/bin/sh

real_pid=`pgrep -P 1 -f /usr/sbin/ntpd`
test_pid=`cat /var/run/ntpd.pid 2>/dev/null`

sync_systemclock () {
        /usr/bin/timeout -s SIGTERM --kill-after=60 30 /usr/sbin/ntpd -gq
        NTPSRV=$(awk '{if($1~/^server/){print $2}}' /etc/ntp.conf | head -n 1)
        if [ -n "$NTPSRV" ] ; then
            /usr/bin/timeout -s SIGTERM --kill-after=15 10 /usr/sbin/ntpdate -u $NTPSRV
        fi
}

if [ "$test_pid" = "$real_pid" ]; then
    if [ "$(ntpq -pn 2>/dev/null | grep -c '^[*+]')" = 0 ]; then
        logger -p daemon.err "ntpd doesn't respond to ntpq, restarting"
        service ntp stop
        sleep 5
        pkill -9 -P 1 -f /usr/sbin/ntpd
        sleep 1
        sync_systemclock
        service ntp restart
    fi
elif [ -z "$real_pid" ]; then
    logger -p daemon.err "ntpd not running, starting"
    sync_systemclock
    service ntp restart
else
    logger -p daemon.err "ntpd broken, restarting"
    service ntp stop
    sleep 5
    pkill -9 -P 1 -f /usr/sbin/ntpd
    sleep 1
    sync_systemclock
    service ntp restart
fi

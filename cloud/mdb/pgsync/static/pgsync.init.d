#!/bin/sh
#
# chkconfig: 2345 99 01
# description: pgsync

### BEGIN INIT INFO
# Provides:          pgsync
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start pgsync
# Description:       Start pgsync
### END INIT INFO

CONFIG=/etc/pgsync.conf
PIDFILE=$(awk '/^pid_file/ {print $NF}' $CONFIG)
STOPFILE="$(awk '/^working_dir/ {print $NF}' $CONFIG)/pgsync.stopped"

start() {
    if status >/dev/null 2>&1
    then
        echo "Already running"
        return 0
    else
        # Removing stale pidfile
        rm -f ${PIDFILE}

        echo -n "Starting pgsync: "
        ulimit -n 1024
        mkdir -p /var/run/pgsync/
        mkdir -p /var/log/pgsync/
        chown -R postgres:postgres /var/run/pgsync/
        chown -R postgres:postgres /var/log/pgsync/
        start-stop-daemon -c postgres --exec /usr/local/bin/pgsync --start
        sleep 1
        if status >/dev/null 2>&1
        then
            echo "OK."
            return 0
        else
            echo "FAIL"
            return 1
        fi
    fi
}

stop() {
    if ! status >/dev/null 2>&1
    then
        echo "Already stopped"
    else
        echo -n "Stopping pgsync: "
        kill $(cat "$PIDFILE")
        sleep 1
        if ! status >/dev/null 2>&1
        then
            echo "OK."
        else
            echo "FAIL"
            kill -9 $(cat "$PIDFILE")
            echo "Killing pgsync: OK"
        fi
    fi

    return 0
}

status() {
    echo -n "pgsync is "
    if [ -f "$PIDFILE" ]
    then
        if kill -0 $(cat "$PIDFILE")
        then
            echo "running (with pid $(cat $PIDFILE))"
            return 0
        else
            echo "not running"
            return 1
        fi
    else
        echo "not running"
        return 1
    fi
}

case "$1" in
  start)
    rm -f ${STOPFILE} && start
    ;;
  stop)
    touch ${STOPFILE} && stop
    ;;
  restart)
    rm -f ${STOPFILE} && stop && start
    ;;
  status)
    status
    ;;
  force-reload)
    stop && start
    ;;
  *)
    echo "$(basename $0) {start|stop|status|restart|force-reload}"
    exit 1
esac
exit $?

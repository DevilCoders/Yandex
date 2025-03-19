#!/bin/sh
#
# chkconfig: 2345 99 01
# description: dbaas-cron

### BEGIN INIT INFO
# Provides:          dbaas-cron
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start dbaas-cron
# Description:       Start dbaas-cron
### END INIT INFO

CONFIG=/etc/dbaas-cron/dbaas-cron.conf
PIDFILE=/var/run/dbaas-cron.pid
USER=monitor
NAME=dbaas-cron
LOGDIR=/var/log/dbaas-cron

OPTS="--pidfile ${PIDFILE} --quiet"
PROGRAM_OPTS="-c ${CONFIG}"

start() {
    if status >/dev/null 2>&1
    then
        echo "Already running"
        return 0
    else
        # Removing stale pidfile
        rm -f ${PIDFILE}

        mkdir -p ${LOGDIR}
        chown -R ${USER}:${USER} ${LOGDIR}

        echo -n "Starting ${NAME}: "
        start-stop-daemon ${OPTS} -c ${USER} --background --make-pidfile \
            --exec /opt/yandex/dbaas-cron/bin/python --start -- \
            /opt/yandex/dbaas-cron/bin/dbaas_cron ${PROGRAM_OPTS}
        sleep 1
        if status >/dev/null 2>&1
        then
            echo "OK."
            rm -f ${STOPFILE}
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
    start-stop-daemon ${OPTS} --stop --verbose --retry TERM/10
    fi

    return 0
}

status() {
    start-stop-daemon ${OPTS} --status
    STATUS=$?
    echo -n "${NAME} is "
    if [ $STATUS -eq 0 ]
    then
        echo "running"
    else
        echo "not running"
    fi
    return $STATUS
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    stop && start
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

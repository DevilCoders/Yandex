#!/bin/bash

if service dbaas-cron status >/dev/null 2>&1
then
    delta=$(($(date +%s) - $(stat -c %Z /tmp/dbaas-cron.status)))
    if [ ${delta} -gt {{ salt['pillar.get']('data:dbaas_cron:status_timeout', 90) }} ]
    then
        pkill --signal SIGKILL -f /opt/yandex/dbaas-cron/bin/dbaas_cron
        service dbaas-cron start >/dev/null 2>&1
        logger -p daemon.err "stale dbaas-cron restarted"
    fi
    count=$(pgrep -c -f /opt/yandex/dbaas-cron/bin/dbaas_cron)
    if [ "${count}" -gt 1 ]
    then
        pkill --signal SIGKILL -f /opt/yandex/dbaas-cron/bin/dbaas_cron
        service dbaas-cron restart >/dev/null 2>&1
    fi
else
    service dbaas-cron restart >/dev/null 2>&1
    logger -p daemon.err "dbaas-cron restarted"
fi

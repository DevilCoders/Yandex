#!/bin/bash

set -x

FLAG_PATH=/etc/cron.yandex/initial-salt-deploy.done
HEARTBEAT_PATH=/etc/cron.yandex/heartbeat.py
ERRATA_APPLY_PATH=/usr/local/sbin/errata_apply.sh
RESTART_REQURED_PATH=/usr/local/yandex/monitoring/restart_required.py

if [ -f "${FLAG_PATH}" ]
then
    exit 0
fi

SYNC_RET=""

while [ "$SYNC_RET" != "0" ]
do
    /usr/bin/salt-call saltutil.sync_all
    SYNC_RET="$?"
done

COMMON_ONLY_HS_RET=""

while [ "$COMMON_ONLY_HS_RET" != "0" ]
do
    /usr/bin/salt-call \
        --retcode-passthrough \
        --out=highstate \
        --state-out=changes \
        --output-diff \
        --log-level=quiet \
        --log-file-level=info \
        state.highstate \
        queue=True \
        pillar='{data: {runlist: [components.common]}}'
    COMMON_ONLY_HS_RET="$?"
done

if [ -x "${ERRATA_APPLY_PATH}" ]
then
    EA_RET=""
    while [ "$EA_RET" != "0" ]
    do
        ${ERRATA_APPLY_PATH}
        EA_RET="$?"
    done
fi

if [ -x "${RESTART_REQURED_PATH}" ]
then
    if [ "$(${RESTART_REQURED_PATH})" != "PASSIVE-CHECK:restart_required;0;OK" ]
    then
        /sbin/reboot
    fi
fi

HS_RET=""

while [ "$HS_RET" != "0" ]
do
    /usr/bin/salt-call \
        --retcode-passthrough \
        --out=highstate \
        --state-out=changes \
        --output-diff \
        --log-level=quiet \
        --log-file-level=info \
        state.highstate \
        queue=True \
        pillar='{target-container: no-container}'
    HS_RET="$?"
done

if [ -f "${HEARTBEAT_PATH}" ]
then
    SI_RET=""
    while [ "$SI_RET" != "0" ]
    do
        /usr/bin/salt-call \
            state.apply \
            components.dom0porto.server_info
        SI_RET="$?"
    done

    HB_RET=""

    while [ "$HB_RET" != "0" ]
    do
        /bin/sleep 5
        ${HEARTBEAT_PATH}
        HB_RET="$?"
    done

    HS2_RET=""
    while [ "$HS2_RET" != "0" ]
    do
        /usr/bin/salt-call \
            --retcode-passthrough \
            --out=highstate \
            --state-out=changes \
            --output-diff \
            --log-level=quiet \
            --log-file-level=info \
            state.highstate \
            queue=True
        HS2_RET="$?"
    done
fi

/usr/bin/touch "${FLAG_PATH}"

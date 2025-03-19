#!/bin/bash

SYSTEMD_PAGER=''

cat /etc/monrun/MANIFEST.json | python -m json.tool 2>/dev/null
if [ $? -ne 0 ]; then
    chown -R monitor:monitor /etc/monrun && sudo -u monitor monrun --gen-jobs
    logger -p daemon.err "invalid monrun manifest regenerated"
fi

if service juggler-client status
then
    newest=$(ls -t /var/cache/monrun/ | head -n1)
    delta=$(($(date +%s) - $(stat -c %Z "/var/cache/monrun/${newest}")))
    if [ ${delta} -gt 90 ]
    then
        service juggler-client restart
        logger -p daemon.err "stale juggler-client restarted"
    fi
else
    service juggler-client restart
    logger -p daemon.err "juggler-client restarted"
fi

#!/bin/bash

# RSYNC_PASSWORD="GC1WwPld" rsync yandexrunet@mirrors.kernel.org::t2fedora-enchilada

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="fedora"
#HOST="ftp.nluug.nl"
#HOST="fedora.uib.no"
#HOST="ftp.halifax.rwth-aachen.de"
#HOST="ftp.acc.umu.se"
#HOST="ftp-stud.hs-esslingen.de"
#HOST="mirror.liteserver.nl"
#HOST="mirror.23media.com"
HOST="download-cc-rdu01.fedoraproject.org"
#MODULE="mirror/fedora/enchilada"
#MODULE="fedora"
MODULE="fedora-enchilada"

EXCLUDE="
--exclude=core/
--exclude=releases/
--exclude=extras/
--exclude=atrpms/
--exclude=stage/
--exclude=elrepo/
--exclude=dag/
--exclude=rpmfusion/
--exclude=russianfedora/
--exclude=tedora/
--exclude=tigro/
--exclude=lasttransfer"

REMOTE_TRACE="fullfilelist"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    setstamp
fi

compresslog

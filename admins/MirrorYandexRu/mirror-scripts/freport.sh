#!/bin/sh

LOCK="/var/lock/report_mirror"

if [ -f $LOCK ]; then
    echo "report_mirror already run. Abort..."
    exit 1;
fi


touch $LOCK

DATE=$(date +%Y%m%d-%H%M)

/usr/local/bin/report_mirror -c /etc/report_mirror.conf -d -o /var/log/mirrors/fedora-report-ftp-$DATE.log

bzip2 -9 /var/log/mirrors/fedora-report-ftp-$DATE.log

rm -f $LOCK

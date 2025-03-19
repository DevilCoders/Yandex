#!/bin/bash

hostname=`hostname -s` ;
hosttype='mulca';

if [ "$hosttype" = "No hosts found" ]
then
        echo "Can't detect hosttype"
        exit 1
elif [ x"$hosttype" = "x" ]
then
        echo "Empty hosttype"
        exit 1
fi

date=`/bin/date --date=yesterday +%Y%m%d` ;
logspooldir="/var/log/disk-logs" ;
logstorehost="logstore.disk.yandex.net"
backup_log=/var/log/logbackup.log
# rsync config
rsync_user=disk
rsync_pwdfile=/etc/downloader-logbackup-rsync.passwd
rsync_area=backup

logs=(
/var/log/nginx/downloader/access.log
/var/log/nginx/downloader/error.log
/var/log/downloads_counter.log
)


# Check, if spooldir exists
if [ ! -d $logspooldir ] ; then
    mkdir -p $logspooldir; 
fi

# Check curl is installed
if ! [ -x "/usr/bin/curl" ]; then
        echo "Please, install curl on `hostname`"
        exit 0
fi

# Prepare log for upload
for log in ${logs[@]} ; do
    if [ -f $log.0 ] ; then
        logname=`ls -1 $log.0 |awk -F "/var/log/" '{print $2}' |sed -e "s/\//\_/g" |sed -e "s/\.0//"` ;
        gzip -c $log.0 > $logspooldir/$logname.$date.gz
    elif [ -f $log.0.gz ] ; then
        logname=$(echo ${log#/var/log/} | sed 's|/|_|g')
        cp "$log.0.gz" "$logspooldir/$logname.$date.gz"
    fi
done

# Sleep for async
sleep $((RANDOM%3600+90))

# Upload the staff using rsync
if [ -n "`ls -1 $logspooldir/`" ]; then 
    rsync -ptuqh --log-file=$backup_log --password-file=$rsync_pwdfile $logspooldir/* rsync://${rsync_user}@${logstorehost}/${rsync_area}/$hosttype/$hostname/ >/dev/null 2>&1 && \
    (rm -f $logspooldir/*) || \
    echo "upload failed"
fi

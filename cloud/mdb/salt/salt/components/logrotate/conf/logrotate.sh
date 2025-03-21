#!/bin/sh

test -x /usr/sbin/logrotate || exit 0

# Clean non existent log file entries from status file
cd /var/lib/logrotate
test -e status || touch status
head -1 status > status.clean
sed 's/"//g' status | while read logfile date
do
    [ -e "$logfile" ] && echo "\"$logfile\" $date"
done >> status.clean
mv status.clean status

/usr/sbin/logrotate /etc/logrotate.conf

#Remove status file
if [ $? -ne 0 ]
then
    rm -f status
fi

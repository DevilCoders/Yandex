#!/bin/sh
for i in $(ls -1 /opt/kp/space01h.kp.yandex.net/kp-front/var/log/nginx/*.log.2.gz | grep -v 'error'); do
d=$(date  '+%d%m%y')
remfname=$(echo ${i} | awk -F'/' '{print $NF}' |  sed  "s/2\.gz/${d}\.2\.gz/g")
   rsync -qrt -e 'ssh -i /etc/seclog/id_dsa -l incoming'  --password-file=/etc/seclog/rsync.seclog.passwd $i rsync://yandex@incoming.seclog.ru/yandex/fe2/${remfname}
   echo $? > /tmp/seclog-rsync-status 
done

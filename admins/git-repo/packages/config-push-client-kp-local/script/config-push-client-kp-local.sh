#!/bin/bash

#list of logs
#LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]$)'` ;
#LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]+$|\.PREV$|\/kern\.log|\/dmesg|\/mail\.|\/wtmp|\/atop\.log|\/fsck\/|\/fixmtu\.log|\/statbox.*|\/news\/news\.|\/lpr\.|\/btmp|\/lastlog$|\/glusterfs\/|\/auth\.|\/messages$|\/boot$|zabbix.*|\/snaked|juggler.*|\/faillog$)'`
LIST=`find /var/log -type f | egrep -v '(boot|tinyproxy*|conductor-agent|user.log*|lastlog|kern.log|pkgver|logrotate|alternatives|apt|cron|syslog|monrun|agave|dpkg|debug$|error$|daemon|\.gz$|\.[0-9]+$|\.PREV$|\/kern\.log|\/dmesg|\/mail\.|\/wtmp|\/atop\.log|\/fsck\/|\/fixmtu\.log|\/statbox.*|\/news\/news\.|\/lpr\.|\/btmp|\/lastlog$|\/glusterfs\/|\/auth\.|\/messages$|\/boot$|zabbix.*|\/snaked|juggler.*|\/faillog$)'`
#hostname
H=`hostname -f`
#default cgroup
DGROUP=`curl --connect-timeout 10 -s http://c.yandex-team.ru/api-cached/generator/paulus.aggregation_group?fqdn=$H || cat /var/tmp/$H`
#config file
CFILE=/tmp/config-push-client-kp-local.yaml
CFILE2=/tmp/config-push-client-kp-local2.yaml
CFILE_AGGREGATE='/tmp/config-push-client-kp-local-aggregate.yaml'

echo '' > $CFILE ;

cat /etc/yandex/statbox-push-client/config-push-client-kp-local.yaml > $CFILE;

for i in $LIST ; do
 echo " - name: $i" >> $CFILE
 echo "   fakename: /$DGROUP/$H/$i" >> $CFILE
 echo "   send_delay: 10" >> $CFILE
done;

cat '/etc/yandex/statbox-push-client/config-push-client-kp-local-aggregate.yaml' > $CFILE_AGGREGATE;

for i in $LIST ; do
 echo " - name: $i" >> $CFILE_AGGREGATE
 echo "   fakename: /$DGROUP/$i" >> $CFILE_AGGREGATE
 echo "   send_delay: 10" >> $CFILE_AGGREGATE
done;

cp $CFILE $CFILE2
sed -i s/space01h/space01e/g $CFILE2

#!/bin/bash

#list of logs
#LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]$)'` ;
LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]+$|\.PREV$|\/kern\.log|\/dmesg|\/mail\.|\/wtmp|\/atop\.log|\/fsck\/|\/fixmtu\.log|\/statbox.*|\/news\/news\.|\/lpr\.|\/btmp|\/lastlog$|\/glusterfs\/|\/auth\.|\/messages$|\/boot$|zabbix.*|\/snaked|juggler.*|\/faillog$|lighttpd-x5\/error\.log)'`
#hostname
H=`hostname -f`
#default cgroup
DGROUP=`curl http://c.yandex-team.ru/api/generator/paulus.aggregation_group?fqdn=$H`
#config file
CFILE=/tmp/config-push-client-corba-local.yaml

echo '' > $CFILE ;

cat /etc/yandex/statbox-push-client/config-push-client-corba-local.yaml > $CFILE;

for i in $LIST ; do
 echo " - name: $i" >> $CFILE
 echo "   fakename: /$DGROUP/$H/$i" >> $CFILE
 echo "   send_delay: 300" >> $CFILE
done;

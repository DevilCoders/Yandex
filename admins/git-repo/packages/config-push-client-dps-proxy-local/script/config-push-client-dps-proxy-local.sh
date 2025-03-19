#!/bin/bash

#list of logs
#LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]$)'` ;
LIST=`find /var/log -type f | egrep -v '(\.gz$|\.[0-9]+$|\.log\-[0-9]+\-[0-9]+$|\.PREV$|\/kern\.log|\/dmesg|\/mail\.|\/wtmp|\/atop\.log|\/fsck\/|\/fixmtu\.log|\/statbox.*|\/news\/news\.|\/lpr\.|\/btmp|\/lastlog$|\/glusterfs\/|\/auth\.|\/messages$|\/boot$|zabbix.*|\/snaked|juggler.*|\/faillog$)'`
#hostname
H=`hostname -f`
#default cgroup
DGROUP=`curl http://c.yandex-team.ru/api/generator/paulus.aggregation_group?fqdn=$H`
#config file
CFILE=/tmp/config-push-client-dps-proxy-local.yaml
CFILE_AGGREGATE='/tmp/config-push-client-dps-proxy-local-aggregate.yaml'

echo '' > $CFILE ;

cat '/etc/yandex/statbox-push-client/config-push-client-dps-proxy-local.yaml' > $CFILE;

for i in $LIST ; do
 echo " - name: $i" >> $CFILE
 echo "   fakename: /$DGROUP/$H/$i" >> $CFILE
 echo "   send_delay: 10" >> $CFILE
done;

cat '/etc/yandex/statbox-push-client/config-push-client-dps-proxy-local-aggregate.yaml' > $CFILE_AGGREGATE;

for i in $LIST ; do
 echo " - name: $i" >> $CFILE_AGGREGATE
 echo "   fakename: /$DGROUP/$i" >> $CFILE_AGGREGATE
 echo "   send_delay: 10" >> $CFILE_AGGREGATE
done;

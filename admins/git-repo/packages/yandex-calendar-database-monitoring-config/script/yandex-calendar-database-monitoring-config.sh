#!/bin/bash

h=`hostname -f`
DESC='';
caldav=22280
web=22281
worker=22282

for i in `ls /etc/yandex/calendar/nature/ | grep "web\|caldav\|worker"` ; do
  p="$[${i}]" ;
  rm -f /tmp/curl.cal.6193.$p ;
  curl -s http://${h}:${p}/dbs| grep -v '?' | tr '|' ' ' | grep -v '\-\-' | grep -v 'M' > /tmp/curl.cal.6193.$p ;
  DESC=`echo $DESC ; if [ ! -s /tmp/curl.cal.6193.$p ] ; then echo "could not open http://${h}:${p}/dbs" ;
  else cat /tmp/curl.cal.6193.$p | while read ms name url status open idle ; do
   if [ $status != 'available' ] ; then
    echo $url $status ; fi ; done ; fi`
#for debug
#  echo $DESC;
#for debug
#echo $i;
done ;

D=`echo $DESC`
if [ -z "$D" ] ;then echo '0;ok' ; else echo "2;$D" ; fi
#for debug
#echo $D
#$DESC


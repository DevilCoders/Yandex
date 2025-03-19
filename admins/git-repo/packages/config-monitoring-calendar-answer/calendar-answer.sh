#!/bin/bash

ST=0;
DESC='';
mkdir -p /tmp/var/log/corba/
for L in /var/log/corba/calendar.web.log /var/log/corba/calendar.caldav.log ; do
#echo $L;
## Get last 5 minutes
  /usr/bin/timetail -n 14700 -t java $L | grep HTTP > /tmp/${L}
  str=`wc -l /tmp/${L} | awk '{print $1}'`
  for i in `cat /tmp/${L} | sed 's/.*HTTP\/1.\+\"\(.*\)/\1/g' | awk '{print $1}'  | sort -u | grep -v 200` ; do
## Non-200 answer codes
#    echo $i ;
## count of each code
    p1=`cat /tmp/${L} | grep -c " $i "`;
    DESC="$DESC-in ${L} count of $i:$p1 ";
#    echo $p1;
## convert 50X => 500
    j=`echo $i | sed 's/50./500/g'`
#    echo $j;
    if [ $p1 -ge 100 ] && [ $j -eq 500 ]; then ST=2; fi ;
  done;
done ;

echo "$ST;$DESC";


#!/bin/bash
DESC='';
ST=0;
for i in `ls /etc/yandex/pinger.d/` ;
  do
    DESC="$DESC ${i}-`/usr/sbin/pingone3 c_${i}`";
    if [[ $? -ne 0 ]] ;
     then ST=2 ;
     else ST=$ST ; fi ;
  done
echo "$ST; $DESC"


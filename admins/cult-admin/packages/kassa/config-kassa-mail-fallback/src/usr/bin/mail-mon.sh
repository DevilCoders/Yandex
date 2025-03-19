#!/bin/bash
rmail=$(timetail -n 1800  -t syslog /var/log/mail-fallback.log  | grep Remove | wc -l)
curmail=$(find /var/spool/postfix/deferred/ /var/spool/postfix/active/ /var/spool/postfix/maildrop/ -type 'f' | wc -l)
all=$(($rmail + $curmail))
if [ $all -gt 10 ]; then
   echo "1; Delete last halfhour and current mail is $all"
elif [ $all -gt 30 ]; then
   echo "2; Delete last halfhour and current mail is $all"
else
  echo "0; ok. Delete last halfhour and current mail is $all"
fi

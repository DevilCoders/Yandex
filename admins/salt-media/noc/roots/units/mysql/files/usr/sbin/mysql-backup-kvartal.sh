#!/bin/bash
if [ ! -e "/backup_kvartal" ]
then
    mkdir /backup_kvartal
fi
# make backup only on replica
if [ $(mysync state -s | grep $(hostname -f).*repl=master | wc -l) -ne 1 ]
then
    sleep $((RANDOM % 1200))
    xtrabackup --defaults-file=/etc/mysql/my.cnf -u admin -p $(cat /root/.my.cnf | grep password | sed 's/password=//g') --lock-ddl-per-table  --lock-ddl-timeout=3600 --datadir=/var/lib/mysql --target-dir=/backup_kvartal/$(date '+%Y%m') --backup --compress
fi

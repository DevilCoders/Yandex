#!/bin/bash
if [ ! -e "/backup" ]
then
    mkdir /backup
fi
# make backup only on replica
if [ $(mysync state -s | grep $(hostname -f).*repl=master | wc -l) -ne 1 ]
then
    sleep $((RANDOM % 600))
    if [ ! -e "/backup/full" ]
    then
        # please remove /backup/full of /backup directory to make new full backup
        rm -r /backup/*
        # make full backup
        xtrabackup --defaults-file=/etc/mysql/my.cnf -u admin -p $(cat /root/.my.cnf | grep password | sed 's/password=//g') --lock-ddl-per-table  --lock-ddl-timeout=3600 --datadir=/var/lib/mysql --target-dir=/backup/full --backup --compress
    else
        # make incremental backup
        xtrabackup --defaults-file=/etc/mysql/my.cnf -u admin -p $(cat /root/.my.cnf | grep password | sed 's/password=//g') --lock-ddl-per-table  --lock-ddl-timeout=3600 --datadir=/var/lib/mysql --target-dir=/backup/$(date '+%Y%m%d') --incremental-basedir=/backup/full --backup --compress
    fi
    # delete old incremental backup
    find /backup -name "20[0-9]*" -mtime +21 -type d -exec rm -rv {} \+
    # kostyl by borislitv, zaparili zvonit nochiu
    if [ $(df -h / | tail -n 1 | awk '{print $5}' | grep -o "[0-9]*") -gt 70 ]
    then
        find /backup -name "20[0-9]*" -mtime +7 -type d -exec rm -rv {} \+
    fi
fi

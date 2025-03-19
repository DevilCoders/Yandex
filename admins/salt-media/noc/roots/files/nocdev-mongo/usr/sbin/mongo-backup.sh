#!/bin/bash
set -e
HOST=$(hostname -f)
if [ $(mongo -u root -p {{pillar['mongo-root']['chk-mongo-root2']}} --eval 'rs.conf()' | grep 'hidden\" : true' -B 3 | grep $HOST | wc -l) -eq 1 ]
then
    mkdir -p /backup/
    cd /backup
    mongodump --gzip -o /backup/$(date "+%Y-%m-%d-%H-%M-%S") -u root -p {{pillar['mongo-root']['chk-mongo-root2']}}
    find /backup/ -mtime +21 -type d -exec rm -rv {} \+
fi

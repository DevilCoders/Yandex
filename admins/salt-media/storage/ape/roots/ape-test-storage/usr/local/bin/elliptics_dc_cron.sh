#!/bin/bash
if [ `find /run/lock/elliptics_dc.lock -type f -mtime -6 | wc -l` == 0 ]
then
    rm -rv /var/tmp/dnet_recovery_dc
    sleep $((RANDOM % 12000))
    dnet_recovery -r elisto01f.tst.ape.yandex.net:1025:10 -r elisto01i.tst.ape.yandex.net:1025:10 -n 1 -l /dev/null -u -g 3,4 dc
    touch /run/lock/elliptics_dc.lock
fi

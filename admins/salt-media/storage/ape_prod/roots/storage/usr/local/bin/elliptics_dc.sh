#/bin/bash
rm -rv /var/tmp/dnet_recovery_dc
ubic info elliptics | awk '{print $1}' | grep = | cut -d= -f2 | grep -v 541 | while read backend;
do
    rm -rv /var/tmp/dnet_recovery_dc
    dnet_recovery -n 4 -i $backend -o `hostname -f`:1025:10 -r elisto04e.ape.yandex.net:1025:10 -r elisto05e.ape.yandex.net:1025:10 -r elisto06e.ape.yandex.net:1025:10 -r elisto01h.ape.yandex.net:1025:10 -r elisto02h.ape.yandex.net:1025:10 -r elisto03h.ape.yandex.net:1025:10 -l /dev/null -u -g 1,4 dc
done
touch /run/lock/elliptics_dc.lock

#!/bin/bash

/usr/sbin/iptruler all down
sleep 120
/etc/init.d/nginx stop;
ubic stop mediastorage-proxy
echo "nginx stopped on `hostname -f`" | mail -s "free_space_watchdog" mds-cc@yandex-team.ru;

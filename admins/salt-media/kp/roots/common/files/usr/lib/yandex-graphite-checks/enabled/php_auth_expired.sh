#!/bin/bash
# MANAGED BY SALT common/files/usr/lib/yandex-graphite-checks
# CADMIN-6663
export PATH=/usr/local/bin:/usr/local/sbin:/usr/bin:/usr/sbin:/bin:/sbin
echo -n "count "
timetail -r '^\[(\S+\s+\S+)\]' -n60 /var/log/php/php-main.log | grep -c 'Authentication expired'

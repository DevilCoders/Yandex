#!/bin/sh
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
timetail -n 60 -r "^(\S+\s+\S+)" /var/log/nginx/error.log | perl /usr/lib/yandex-graphite-checks/scripts/nginx-error-log-analyzer.pl

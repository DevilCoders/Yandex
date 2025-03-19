#!/usr/bin/env bash

status_file='/var/run/mdb-katan/imp/last-exit-status'

if [[ ! -e $status_file ]]
then
    echo "0;OK"
    exit 0
fi


last_status=$(cat "$status_file")

if [[ "$last_status" == "0" ]]
then
    echo '0;OK'
    exit 0
fi

last_error=$(grep level=ERROR /var/log/mdb-katan/imp/stdout.log | grep -Eo 'msg=.*' | tail -n1 | tr '\n;' ' .')

# 2 is 'uninitialized' exit code (https://a.yandex-team.ru/review/1235212)
if [[ "$last_status"  == "2" ]]
then
    echo "1;WARN;$last_error"
    exit 0
fi

echo "2;CRIT;$last_error"


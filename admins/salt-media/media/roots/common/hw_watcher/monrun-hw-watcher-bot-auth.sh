#!/bin/bash

level=0
msg=""

watch_window=3600
bad_string='Failed request on https://bot.yandex-team.ru/api/v3/dc/request'

for log in /var/log/hw_watcher/*.log; do
    if ! test -e "$log"; then
        continue
    fi

    file_name="${log##*/}"
    fail_count="$(timetail -t java -n$watch_window "$log" | grep -F "$bad_string" -c || true)"
    if ((fail_count > 0)); then
        level=2
        msg="$msg${msg:+, }Bot request fail #$fail_count times for ${file_name%.log}"
    fi
done

echo "$level;${msg:-OK}"

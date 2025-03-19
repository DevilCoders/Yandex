#!/usr/bin/env bash

HOST=`hostname -f`
TR=`curl -s http://c.yandex-team.ru/api-cached/groups2hosts/kp-trailers|grep -v $HOST|xargs`

for i in $TR; do ionice -c 3 rsync --bwlimit 10000 -a /home/www/ $i:/home/www/; done

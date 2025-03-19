#!/bin/bash

if [ "$(host -t cname pull-mirror.yandex.net | awk '{print $NF}')" = "$(hostname -f)." ];
	then echo "0;I'm a master, don't need to check"
else
	/usr/bin/monrun_http_code.sh nginx "((^|:)([0-9a-fA-F]{0,4})){1,8}" "/.ping/ping|/.ok.txt|/ping|/timetail" "200" -ge 10 2	
fi


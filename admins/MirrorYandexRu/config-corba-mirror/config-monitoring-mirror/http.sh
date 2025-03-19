#!/bin/bash

if [ "$(host -t cname pull-mirror.yandex.net | awk '{print $NF}')" = "$(hostname -f)." ];
	then echo "0;I'm a master, don't need to check"
else
	/usr/bin/http_check.sh .ok.txt 80
fi


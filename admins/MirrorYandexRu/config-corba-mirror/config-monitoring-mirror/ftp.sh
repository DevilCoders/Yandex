#!/bin/bash

if [ "$(host -t cname pull-mirror.yandex.net | awk '{print $NF}')" = "$(hostname -f)." ];
	then echo "0;I'm a master, don't need to check"
else
	answer=$(curl --max-time 3 ftp://localhost/.ok.txt -H mirror.yandex.ru -w "code: %{http_code}" -o /dev/null 2>/dev/null)
	if [ "$answer" == "code: 226" -o "$answer" == "code: 150" ]
		then echo "PASSIVE-CHECK:http;0;ok"
	else 
		echo "PASSIVE-CHECK:http;2;$answer"
	fi
fi


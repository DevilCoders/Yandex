#!/bin/bash
answer=$(rsync --timeout=5 --contimeout=5 rsync://mirror.yandex.ru/ubuntu-cdimage 2>&2 1>/dev/null)
if [[ $? != "0" ]]
	then echo "PASSIVE-CHECK:http;2; Connection to rsync failed"
else 
	echo "PASSIVE-CHECK:http;0;ok"
fi


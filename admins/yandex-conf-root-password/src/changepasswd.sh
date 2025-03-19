#!/bin/bash

#detecting environment
. /usr/local/sbin/autodetect_environment

PROJECT=$(curl -sk https://c.yandex-team.ru/api/hosts2projects/`hostname -f`)

if [ "x$PROJECT" == "x" ]; then
    echo Cannot get project name from conductor
    exit -1
fi

if [ -f /usr/share/yandex-configs/yandex-conf-root-password/hashpass.$PROJECT ]; then
    FILENAME="hashpass.$PROJECT"
else
    FILENAME="hashpass"
fi

if [ "$is_virtual_host" -eq 1 -a "$is_kvm_host" -eq 0 ]
	then passwd -d root		#prohibition root password for OpenVZ CT
	else usermod -p `cat /usr/share/yandex-configs/yandex-conf-root-password/$FILENAME` root #changing root password to new
fi

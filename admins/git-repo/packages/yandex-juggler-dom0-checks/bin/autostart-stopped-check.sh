#!/bin/bash

# сей ужасный код проверяет, что у всех остановленных виртуалок выключен автостарт при загрузке хоста.
# работает с lxc и ovz
# http://st.yandex-team.ru/ADMINTASKS-4913

check_lxc () { 
    crit=""
    for i in $(lxctl 2>/dev/null list | grep stopped | awk '{print $NF}'); do
	vmname=$i
	autostart_state=$(cat /etc/lxctl/${vmname}.yaml | grep ^autostart | awk '{print $NF}');
	if [[ ${autostart_state} == "1" ]]; then
	        crit=$(echo -n "$crit $i")
	elif [[ ${autostart_state} == "0" ]]; then
	        true
	else
	        crit=$(echo -n "$crit $i-strange")
	fi;
    done
    
    
    if [[ $crit == "" ]]; then
	    echo "0; ok"
    else
	    echo "2;$crit"
    fi
}

check_ovz () {
    crit=""
    for i in $(vzlist -S -H | awk '{print $1}'); do 
	vmname=$i
	autostart_state=$(cat /etc/vz/conf/${vmname}.conf | grep ^ONBOOT | tr "\"" " " | awk '{print $NF}')
	if [[ ${autostart_state} == "yes" ]]; then
	    crit=$(echo -n "$crit $i")
	elif [[ ${autostart_state} == "no" ]]; then
            true
	else
            crit=$(echo -n "$crit $i-strange")
	fi;
   done
 
    if [[ $crit == "" ]]; then
	echo "0; ok"
    else
	echo "2;$crit"
    fi
}

if [[ -e /usr/bin/lxctl ]]; then
    check_lxc
elif [[ -e /usr/sbin/vzctl ]]; then
    check_ovz
else
    echo "0; ok, i am not dom0-host"
fi



#!/bin/bash

logger () {
    log_file=/var/log/yarobot.log
    while read line; do
	echo "$(date +"[%d/%b/%Y:%H:%M:%S %z]") ${line}" >> ${log_file}; 
    done
}

init () {
    mkdir /var/run/yarobot 1>/dev/null 2>&1
    uptime_seconds=$(cat /proc/uptime  | awk '{print $1}' | sed 's/\..*//g')
    if [[ "300" -ge ${uptime_seconds} ]]; then
	echo "Uptime less then 5 minutes, i will not work now"|logger;
	exit 0;
    fi

    which monrun 1>/dev/null || (echo "There are no monrun installed, nothing to do here"|logger ; return 1) || exit 0

    if [ -e /var/run/yarobot/yarobot.run ]; then 
	echo "run file exists, starting now"|logger;
    else 
	echo "run file does not exists, exiting now"|logger; exit 0;
    fi
}

get_list () {
monrun | grep -v "Type: "
}


check_is_ok () {
if [ -e /usr/share/yarobot/ok_messages/${check} ]; then
    egrep "$(cat /usr/share/yarobot/ok_messages/${check})" 1>/dev/null;
    else
    egrep '=.*(ok|OK|Ok)' 1>/dev/null
fi
}

initial_check () {
    check_is_ok || (echo "${check} seems failed, force cheking" | logger; return 1) && (rm /var/run/yarobot/${check} 1>/dev/null 2>&1; return 0)
}

force_check () {
    monrun -r ${check} 1>/dev/null
}

antiflap () {
    monrun -r ${check} | check_is_ok || (echo "${check} is really not OK, i will try to fix it"|logger; return 1) && echo "${check} seems flapped, will not fix it now" | logger
}

repair_check () {
if [ -e /usr/share/yarobot/${check} ]; then
    /usr/share/yarobot/${check};
    monrun -r ${check} | egrep '=.*(ok|OK|Ok)' 1>/dev/null || (echo "${check} - i was not able to fix it"; return 1) && echo "${check} fixed now" 
    else
    echo "There are no script for ${check}, ignoring it.";
    touch /var/run/yarobot/${check};
fi
}

produce_check () {
if [ -e /var/run/yarobot/${check} ]; then
    echo "There are /var/run/yarobot/${check} file, so i will not work with ${check}"|logger
    else
    antiflap || repair_check|logger;
    fi
}

exclude_list () { 
    # do not work with this checks:
    egrep -v '(conductor_interfaces)'
}

init

get_list | exclude_list | while read line; 
        do check=$(echo $line | awk '{print $1}'); echo ${line} | initial_check || produce_check; 
done

echo "Yet Another Robot completed his work"| logger


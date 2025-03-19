#!/bin/bash

set +x
# ubuntu with syslog-ng
# Use sudo, lookup pid in /var/run/syslog-ng.pid, search for syslog-ng, expect one process
check[1]=$(/usr/local/yandex/monitoring/daemon_alive.sh -s -p /var/run/syslog-ng.pid -d syslog-ng -m 1);
# stock ubuntu with rsyslog
check[2]=$(/usr/local/yandex/monitoring/daemon_alive.sh -s -p /var/run/rsyslogd.pid -d rsyslogd -m 1);

# find the most succesful result
result="";
for c in "${check[@]}"; do
    if [ "${c:0:1}" != "2" ]; then
        # two checks succeding mean we somehow have two different daemons running.
        if [ "x$result" != "x" ]; then
            result="2;two diffent syslog daemons running"
            break
        fi;
        result=$c;
    fi;
done;

if [ "x$result" = 'x' ]; then 
    echo "2;no syslog daemon running";
else
    echo "$result";
fi;

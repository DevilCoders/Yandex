{%- from "components/greenplum/map.jinja" import pxfvars with context -%}
{%- set maj_ver = pxfvars.major_version -%}
#!/bin/sh

real_pid=`pgrep -P 1 -f /opt/greenplum-pxf-{{ maj_ver }}/application/pxf-app`
test_pid=`cat /etc/greenplum-pxf{{ maj_ver }}/run/pxf-app.pid 2>/dev/null`

if [ "$test_pid" = "$real_pid" ]; then
    if [ "$(curl http://localhost:5888/actuator/health 2>/dev/null | grep -c 'UP')" = 0 ]; then
        logger -p daemon.err "something wrong with pxf, restarting"
        service pxf{{ maj_ver }} stop
        sleep 5
        pkill -9 -P 1 -f /opt/greenplum-pxf-{{ maj_ver }}/application/pxf-app
        sleep 1
        service pxf{{ maj_ver }} restart
    fi
elif [ -z "$real_pid" ]; then
    logger -p daemon.err "pxf not running, starting"
    service pxf{{ maj_ver }} restart
else
    logger -p daemon.err "pxf broken, restarting"
    service pxf{{ maj_ver }} stop
    sleep 5
    pkill -9 -P 1 -f /opt/greenplum-pxf-{{ maj_ver }}/application/pxf-app
    sleep 1
    service pxf{{ maj_ver }} restart
fi
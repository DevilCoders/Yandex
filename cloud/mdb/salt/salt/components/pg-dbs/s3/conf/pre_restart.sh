{% from "components/postgres/pg.jinja" import pg with context %}
{# copypasted from salt/components/postgres/resize.sls #}
#!/bin/bash

set -x
export SYSTEMD_PAGER=''

is_in_recovery=$(sudo -u postgres /usr/bin/psql "options='-c log_statement=none -c log_min_messages=panic -c log_min_error_statement=panic -c log_min_duration_statement=-1'" -tAX -c "SELECT pg_is_in_recovery()" 2>/dev/null)

if [[ "$is_in_recovery" = "f" ]]
then
    enable_maintanance=0
    maintenance_status=$(pgsync-util maintenance -m show)
    if [ "$maintenance_status" = "enabled" ]
    then
        enable_maintanance=1
        pgsync-util maintenance -m disable
        sleep 30
    fi
    pgsync-util switchover -y --block -t {{ salt['pillar.get']('data:pgsync:recovery_timeout', '1200') }}
    if [[ "$?" = "0" ]] && [[ "$enable_maintanance" = "1" ]]
    then
        pgsync-util maintenance -m enable
    fi
fi

for service in pgsync {{ pg.connection_pooler }}
do
    if service ${service} status
    then
        service ${service} stop
    fi
done

if sudo -u postgres /usr/local/yandex/pg_wait_started.py -w 3 -m {{ salt['pillar.get']('data:versions:postgres:major_version') }}
then
    sudo -u postgres {{ pg.bin_path }}/psql -c "CHECKPOINT"
    sudo -u postgres {{ pg.bin_path }}/pg_ctl -D {{ pg.data }} -m fast stop -t {{ salt['pillar.get']('data:pgsync:recovery_timeout', '1200') }}
fi

{% from "components/postgres/pg.jinja" import pg with context %}

compute-postgresql-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            is_in_recovery=$(sudo -u postgres /usr/bin/psql "options='-c log_statement=none -c log_min_messages=panic -c log_min_error_statement=panic -c log_min_duration_statement=-1'" -tAX -c "SELECT pg_is_in_recovery()" 2>/dev/null)

            if [[ "$is_in_recovery" = "f" ]] && [[ "$(python -c 'import json; f = open("/etc/dbaas.conf"); d = json.load(f); print(len(d["cluster_nodes"]["ha"]))')" -gt 1 ]]
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
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-postgresql-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            sudo -u postgres /usr/local/yandex/pg_wait_started.py -w {{ salt['pillar.get']('data:pgsync:recovery_timeout', '1200') }} -m {{ salt['pillar.get']('data:versions:postgres:major_version') }} || exit 1
            sudo -u postgres /usr/local/yandex/pg_wait_synced.py -w ${TIMEOUT:-600} || exit 1

            if ! service {{ pg.connection_pooler }} status
            then
                service {{ pg.connection_pooler }} start
            fi
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
compute-postgresql-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            for cron_file in pg_resetup wd-pgsync
            do
                if ls /etc/cron.d/${cron_file} >/dev/null 2>&1
                then
                    mv -f /etc/cron.d/${cron_file} /etc/cron.d/${cron_file}.disabled
                fi
            done

            is_in_recovery=$(sudo -u postgres /usr/bin/psql "options='-c log_statement=none -c log_min_messages=panic -c log_min_error_statement=panic -c log_min_duration_statement=-1'" -tAX -c "SELECT pg_is_in_recovery()" 2>/dev/null)

            if [[ "$is_in_recovery" = "f" ]] && [[ "$(python -c 'import json; f = open("/etc/dbaas.conf"); d = json.load(f); print(len(d["cluster_nodes"]["ha"]))')" -gt 1 ]]
            then
                enable_maintanance=0
                maintenance_status=$(pgsync-util maintenance -m show)
                if [ "$maintenance_status" = "enabled" ]
                then
                    enable_maintanance=1
                    pgsync-util maintenance -m disable
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

            lsof -t +D /var/lib/postgresql | xargs -r kill -SIGKILL
            umount /var/lib/postgresql
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-postgresql-data-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/postgresql" )
            then
                mount /var/lib/postgresql
            fi

            for service in postgresql pgsync {{ pg.connection_pooler }}
            do
                service $service restart
            done

            for cron_file in pg_resetup wd-pgsync
            do
                if ls /etc/cron.d/${cron_file}.disabled >/dev/null 2>&1
                then
                    mv -f /etc/cron.d/${cron_file}.disabled /etc/cron.d/${cron_file}
                fi
            done
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

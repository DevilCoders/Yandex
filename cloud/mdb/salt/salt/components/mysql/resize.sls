{% set osrelease = salt['grains.get']('osrelease') %}
{% from "components/mysql/map.jinja" import mysql with context %}

compute-mysql-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            if [[ $(mysql --defaults-file=/home/mysql/.my.cnf --execute "SHOW SLAVE STATUS" | wc -l) -eq 0 ]] && [[ "$(python -c 'import json; f = open("/etc/dbaas.conf"); d = json.load(f); print(len(d["cluster_nodes"]["ha"]))')" -gt 1 ]]
            then
                enable_maintanance=0
                if [ "$(mysync maintenance get)" = "on" ]
                then
                    enable_maintanance=1
                    mysync maintenance off -w 60s || exit 1
                fi
                mysync switch --from=$(hostname) -w 600s || exit 1
                if [[ "$enable_maintanance" = "1" ]]
                then
                    mysync maintenance on -w 60s || exit 1
                fi
            fi

            for service in mysync mysql
            do
                if service ${service} status
                then
                    service ${service} stop
                fi
            done
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-mysql-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            if [ "$1" = "service_reload" ]
            then
              for service in mysync mysql
              do
                  service ${service} restart
              done
            fi

            my-wait-started -w 60s || exit 1
{% if salt['pillar.get']('data:mysql:use_semisync_replication', True) %}
            if python -c 'import json; f = open("/etc/dbaas.conf"); d = json.load(f); print(d["cluster_nodes"]["ha"])' | grep -q `hostname` && \
               [[ $(mysql --defaults-file=/home/mysql/.my.cnf --execute "SHOW SLAVE STATUS" | wc -l) -gt 0 ]]
            then
                mysql --defaults-file=/home/mysql/.my.cnf --execute "SET GLOBAL rpl_semi_sync_slave_enabled = 1; STOP SLAVE IO_THREAD; START SLAVE IO_THREAD;" || exit 1
                my-wait-synced --wait {{ salt['pillar.get']('post-restart-timeout', 300) | int }}s --defaults-file=/root/.my.cnf --replica-lag={{ salt['pillar.get']('data:mysql:config:mdb_offline_mode_disable_lag', 30) }}s --mysync-info-file=/var/run/mysync/mysync.info || exit 1
            fi
{% endif %}
            mysql --defaults-file=/home/mysql/.my.cnf --execute 'SET GLOBAL offline_mode = 0' || exit 1
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
compute-mysql-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if [[ $(mysql --defaults-file=/home/mysql/.my.cnf --execute "SHOW SLAVE STATUS" | wc -l) -eq 0 ]] && [[ "$(python -c 'import json; f = open("/etc/dbaas.conf"); d = json.load(f); print(len(d["cluster_nodes"]["ha"]))')" -gt 1 ]]
            then
                enable_maintanance=0
                if [ "$(mysync maintenance get)" = "on" ]
                then
                    enable_maintanance=1
                    mysync maintenance off -w 60s || exit 1
                fi
                mysync switch --from=$(hostname) -w 600s || exit 1
                if [[ "$enable_maintanance" = "1" ]]
                then
                    mysync maintenance on -w 60s || exit 1
                fi
            fi

            for service in mysync mysql
            do
                if service ${service} status
                then
                    service ${service} stop
                fi
            done

            lsof -t +D /var/lib/mysql | xargs -r kill -SIGKILL
            umount /var/lib/mysql
            chmod 0 /var/lib/mysql
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-mysql-data-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/mysql" )
            then
                mount /var/lib/mysql
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

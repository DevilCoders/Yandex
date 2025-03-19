compute-ch-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            if service clickhouse-server status
            then
                service clickhouse-server stop
            fi

            echo OK
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-ch-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            if ! service clickhouse-server status
            then
                service clickhouse-server start
            fi

            /usr/local/yandex/ch_wait_started.py || exit 1

            echo OK
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
compute-ch-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if service clickhouse-server status
            then
                service clickhouse-server stop
            fi

            lsof -t +D /var/lib/clickhouse | xargs -r kill -SIGKILL
            umount /var/lib/clickhouse
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-ch-data-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/clickhouse" )
            then
                mount /var/lib/clickhouse
            fi
            if ! service clickhouse-server status
            then
                service clickhouse-server start
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

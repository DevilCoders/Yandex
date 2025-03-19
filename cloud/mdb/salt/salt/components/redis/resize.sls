compute-redis-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            export HOME=/root
            python /usr/local/yandex/ensure_not_master.py
            if service redis-server status
            then
                service redis-server stop
            fi
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh
        - require:
            - file: /usr/local/yandex/ensure_not_master.py

compute-redis-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            if ! service redis-server status
            then
                service redis-server start
            fi

        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
compute-redis-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if service redis-server status
            then
                service redis-server stop
            fi

            lsof -t +D {{ salt.mdb_redis.get_redis_data_folder() }} | xargs -r kill -SIGKILL
            umount {{ salt.mdb_redis.get_redis_data_folder() }}
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

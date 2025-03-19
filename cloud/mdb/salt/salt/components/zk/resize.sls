{% set zk = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set dbaas = salt.pillar.get('data:dbaas', {}) %}

compute-zk-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            if service zookeeper status
            then
                service zookeeper stop
            fi
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-zk-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            MAX_RETRIES=5
            attempt=0
            success_zk=0
            until [ $attempt -ge $MAX_RETRIES ]
            do
               service zookeeper restart
               if /usr/local/yandex/zk_wait_started.py -p {{ zk.config.clientPort }}
               then
                  success_zk=1
                  break
               fi
               attempt=$((attempt+1))
            done
            if [ $success_zk -ne 1 ]; then
               exit 1
            fi
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.dbaas.is_compute() %}
compute-zk-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if service zookeeper status
            then
                service zookeeper stop
            fi

            if grep -qs '/var/lib/zookeeper ' /proc/mounts; then
                lsof -t +D /var/lib/zookeeper | xargs -r kill -SIGKILL
                umount /var/lib/zookeeper
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

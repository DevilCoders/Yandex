compute-es-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            if service elasticsearch status
            then
                service elasticsearch stop
            fi
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

{% if salt.dbaas.is_compute() %}
compute-es-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if service elasticsearch status
            then
                service elasticsearch stop
            fi

            lsof -t +D /var/lib/elasticsearch | xargs -r kill -SIGKILL
            umount /var/lib/elasticsearch
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-es-data-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/elasticsearch" )
            then
                mount /var/lib/elasticsearch
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

compute-kafka-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            if service kafka status
            then
                service kafka stop
            fi
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-kafka-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            if ! service kafka status
            then
                service kafka start
            fi

            /usr/local/yandex/kafka_wait_synced.py -w ${TIMEOUT:-600} || exit $?
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.dbaas.is_compute() %}
compute-kafka-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            PID=$(systemctl show --property MainPID --value kafka)

            /usr/local/yandex/pre_restart.sh || exit $?

            if [ "$PID" -ne "0" ]; then
              # We do not expect kafka shutdown to take long time here because
              # it should have been transfered replicas on previous steps.
              # But still give it 60 secs to stop before killing it with sigkill.
              timeout 60 tail --pid=$PID -f /dev/null
            fi

            lsof -t +D /var/lib/kafka | xargs -r kill -SIGKILL
            umount /var/lib/kafka
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-kafka-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/kafka" )
            then
                mount /var/lib/kafka
            fi

            # Remove "lost+found", otherwise kafka fails to start with error message:
            # ERROR There was an error in one of the threads during logs loading:
            # org.apache.kafka.common.KafkaException: Found directory /var/lib/kafka/lost+found,
            # 'lost+found' is not in the form of topic-partition or topic-partition.uniqueId-delete
            # (if marked for deletion).
            rmdir /var/lib/kafka/lost+found/ || exit $?

            if ! service kafka status
            then
                service kafka start
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

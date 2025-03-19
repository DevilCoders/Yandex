kafka-stop:
    service.dead:
        - name: kafka
        - require_in:
            - service: mdb-kafka-service

kafka-post-restart:
    cmd.run:
        - name: /usr/local/yandex/post_restart.sh
        - require:
            - service: mdb-kafka-service

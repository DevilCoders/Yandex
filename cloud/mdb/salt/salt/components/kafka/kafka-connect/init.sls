/etc/kafka-connect/worker.properties:
    file.managed:
        - template: jinja
        - source: salt://components/kafka/kafka-connect/conf/worker.properties
        - mode: 755
        - makedirs: True


/etc/kafka-connect/connect-log4j.properties:
    file.managed:
        - template: jinja
        - source: salt://components/kafka/kafka-connect/conf/connect-log4j.properties
        - mode: 755
        - makedirs: True
        - require:
            - pkg: mdb-kafka-pkgs


/etc/systemd/system/kafka_connect_worker.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kafka_connect_worker.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload
        - require:
            - service: mdb-kafka-service
            - file: /etc/kafka-connect/worker.properties
            - file: /etc/kafka-connect/connect-log4j.properties


mdb-kafka-connect-worker-service:
    service.running:
        - name: kafka_connect_worker
        - enable: True
        - require:
            - file: /etc/systemd/system/kafka_connect_worker.service


mdb-kafka-connect-worker-service-restart:
    cmd.run:
        - name: service kafka_connect_worker restart
        - require:
            - service: mdb-kafka-connect-worker-service
        - onchanges:
            - pkg: mdb-kafka-s3-sink-connector-pkgs

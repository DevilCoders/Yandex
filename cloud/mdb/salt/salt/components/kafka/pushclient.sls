/etc/pushclient/conf.d/kafka-topics-logs-grpc.conf:
    file.absent

/etc/pushclient/conf.d/kafka-topics-logs-rt.conf:
    file.absent

/etc/pushclient/kafka_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/kafka_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient

statbox-in-kafka-group:
    group.present:
        - name: kafka
        - addusers:
            - statbox
        - system: True
        - require:
            - user: statbox-user

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/kafka_log_parser.py
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf

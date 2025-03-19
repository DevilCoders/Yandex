/etc/vector/vector_kafka.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector_kafka.toml
        - mode: 440
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

vector-in-kafka-group:
    group.present:
        - name: kafka
        - addusers:
            - vector
        - system: True
        - require:
            - pkg: vector-package
        - require_in:
            - service: vector-service

extend:
    vector-validate-conf:
        cmd.run:
            - onchanges:
                - file: /etc/vector/vector_kafka.toml

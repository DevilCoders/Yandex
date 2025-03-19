/etc/vector/vector_clickhouse.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector_clickhouse.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

/etc/vector/test_vector_clickhouse.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector_clickhouse_test.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

extend:
  vector-validate-conf:
    cmd.run:
      - onchanges:
          - file: /etc/vector/vector_clickhouse.toml
          - file: /etc/vector/test_vector_clickhouse.toml
  vector-test-conf:
    cmd.run:
      - onchanges:
          - file: /etc/vector/vector_clickhouse.toml
          - file: /etc/vector/test_vector_clickhouse.toml

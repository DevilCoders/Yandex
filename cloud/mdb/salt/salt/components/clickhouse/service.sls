{% if salt.mdb_clickhouse.has_separated_keeper() %}
clickhouse-keeper:
    service.running:
        - enable: True
        - init_delay: 3
    cmd.run:
        - name: /usr/local/yandex/ch_keeper_wait_started.py -q
        - onchanges:
            - service: clickhouse-keeper
{% endif %}

clickhouse-server:
    service.running:
        - enable: True
        - init_delay: 3
    cmd.run:
        - name: /usr/local/yandex/ch_wait_started.py -q
        - onchanges:
            - service: clickhouse-server

ensure_system_query_log:
    mdb_clickhouse.ensure_system_query_log:
        - require:
            - cmd: clickhouse-server

cleanup_log_tables:
    mdb_clickhouse.cleanup_log_tables:
        - require:
            - cmd: clickhouse-server

clickhouse-stop:
    cmd.run:
        - name: >
            service clickhouse-server stop &&
            timeout 10 bash -c "while pidof clickhouse-server; do sleep 1; done"
        - require_in:
            - service: clickhouse-server

{% if salt.mdb_clickhouse.check_keeper_service() %}
clickhouse-keeper-stop:
    cmd.run:
        - name: >
            service clickhouse-keeper stop &&
            timeout 10 bash -c "while pidof clickhouse-keeper; do sleep 1; done"
        - require_in:
{% if salt.mdb_clickhouse.has_separated_keeper() %}
            - service: clickhouse-keeper
{% endif %}
            - cmd: clickhouse-stop
{% endif %}

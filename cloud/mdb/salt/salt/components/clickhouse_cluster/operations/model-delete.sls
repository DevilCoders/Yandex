include:
    - components.clickhouse.models
    - components.clickhouse.service
    - components.clickhouse.restart

extend:
    clickhouse-models-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

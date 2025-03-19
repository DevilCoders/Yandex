{% set model_name = salt.pillar.get('target-model') %}

include:
    - components.clickhouse.models
    - components.clickhouse.service
    - components.clickhouse.restart

extend:
    clickhouse-models-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

    model_{{ model_name }}:
        fs.file_present:
            - replace : True

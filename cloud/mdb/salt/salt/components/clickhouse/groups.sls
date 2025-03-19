extend:
    cores-group:
        group:
            - require:
                - pkg: clickhouse-packages
            - members:
                - clickhouse
{% if salt.dbaas.is_aws() %}
    monitor-user:
        user:
            - groups:
                  - clickhouse
            - require:
                  - pkg: clickhouse-packages
{% endif %}

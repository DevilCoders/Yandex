include:
    - .main
    - .groups
    - components.logrotate
    - .ch-backup
    - .clickhouse-cleaner
{% if salt.mdb_clickhouse.ssl_enabled() %}
    - .ssl
{% endif %}
{% if salt.pillar.get('service-restart', False) %}
    - .restart
{% endif %}
{% if salt.pillar.get('convert_zero_copy_schema', False) %}
    - .convert-zero-copy-schema
{% endif %}
{% if salt.pillar.get('data:dbaas:cluster') %}
    - .resize
{% endif %}
{% if salt.mdb_clickhouse.has_embedded_keeper() %}
    - components.zk.cleanup
{% endif %}

{% if salt.dbaas.is_aws() %}
    - .init-aws
{% else %}
    - .init-yandex
{% endif %}

{% if salt.pillar.get('service-restart', False) %}
extend:
    clickhouse-stop:
        cmd.run:
            - require_in:
                - pkg: clickhouse-packages
{% endif %}

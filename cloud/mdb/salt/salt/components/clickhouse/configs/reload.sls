clickhouse-reload-config:
{% if salt.mdb_clickhouse.version_cmp('20.3') >= 0 %}
    mdb_clickhouse.reload_config
{% else %}
    test.nop
{% endif %}

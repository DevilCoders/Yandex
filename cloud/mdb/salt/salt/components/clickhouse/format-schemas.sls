{% set format_schemas      = salt.mdb_clickhouse.format_schemas() %}
{% set format_schema_names = format_schemas.keys() | list %}
{% set filter              = salt.pillar.get('target-format-schema') %}
{% set use_service_account = salt.pillar.get('data:service_account_id', None) != None %}

clickhouse-format-schemas-req:
    test.nop

clickhouse-format-schemas-ready:
    test.nop

/var/lib/clickhouse/format_schemas:
    file.directory:
        - user: root
        - group: clickhouse
        - dir_mode: 750
        - require:
            - test: clickhouse-format-schemas-req
        - require_in:
            - test: clickhouse-format-schemas-ready

{% for format_schema_name, format_schema in format_schemas.items() %}
{%     if (not filter) or (format_schema_name == filter) %}
format_schema_{{ format_schema_name }}:
    fs.file_present:
{%         if format_schema['type'] == 'capnproto' %}
        - name: /var/lib/clickhouse/format_schemas/{{ format_schema_name }}.capnp
{%         else %}
        - name: /var/lib/clickhouse/format_schemas/{{ format_schema_name }}.proto
{%         endif %}
        - s3_cache_path: {{ salt.mdb_clickhouse.s3_cache_path('format_schema', format_schema_name) }}
        - url: {{ format_schema['uri'] }}
        - use_service_account_authorization: {{ use_service_account }}
        - decode_contents: True
        - skip_verify: True
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - replace: False
        - require:
            - test: clickhouse-format-schemas-req
        - require_in:
            - test: clickhouse-format-schemas-ready
{%     endif %}
{% endfor %}

{% for ext in ['capnp', 'proto'] %}
cleanup_{{ ext }}_format_schemas:
    fs.directory_cleanedup:
        - name: /var/lib/clickhouse/format_schemas
        - expected: {{ format_schema_names }}
        - suffix: '.{{ ext }}'
        - require:
            - test: clickhouse-format-schemas-req
        - require_in:
            - test: clickhouse-format-schemas-ready
{% endfor %}

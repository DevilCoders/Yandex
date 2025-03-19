{% set custom_geobase_uri          = salt.mdb_clickhouse.custom_geobase_uri() %}
{% set custom_geobase_path         = salt.mdb_clickhouse.custom_geobase_path() %}
{% set custom_geobase_archive_dir  = salt.mdb_clickhouse.custom_geobase_archive_dir() %}
{% set custom_geobase_archive_path = salt.mdb_clickhouse.custom_geobase_archive_path() %}
{% set use_service_account         = salt.pillar.get('data:service_account_id', None) != None %}

clickhouse-geobase-req:
    test.nop

clickhouse-geobase-ready:
    test.nop

{% if custom_geobase_uri %}
custom_geobase_archive:
    fs.file_present:
        - name: {{ custom_geobase_archive_path }}
        - s3_cache_path: {{ salt.mdb_clickhouse.s3_cache_path('geobase', 'custom_clickhouse_geobase') }}
        - url: {{ custom_geobase_uri }}
        - use_service_account_authorization: {{ use_service_account }}
        - skip_verify: True
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - replace: False
        - require:
            - test: clickhouse-geobase-req

custom_geobase:
    archive.extracted:
        - name: {{ custom_geobase_path }}
        - source: {{ custom_geobase_archive_path }}
        - enforce_toplevel: False
        - require:
            - fs: custom_geobase_archive
        - require_in:
            - test: clickhouse-geobase-ready
{% else %}
delete_custom_geobase:
    file.absent:
        - names:
            - {{ custom_geobase_path }}
            - {{ custom_geobase_archive_dir }}
        - require:
            - test: clickhouse-geobase-req
        - require_in:
            - test: clickhouse-geobase-ready

# TODO remove cahched object when custom geobase deletion supported in API

{% endif %}

clickhouse-storage-req:
    test.nop

clickhouse-storage-ready:
    test.nop:
        - on_changes_in:
            - mdb_clickhouse: clickhouse-reload-config

/etc/clickhouse-server/config.d/storage_policy.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_storage_config
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - require:
            - test: clickhouse-storage-req
        - require_in:
            - test: clickhouse-storage-ready

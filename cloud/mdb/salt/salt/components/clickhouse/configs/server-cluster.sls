clickhouse-server-cluster-req:
    test.nop

clickhouse-server-cluster-ready:
    test.nop:
        - on_changes_in:
            - mdb_clickhouse: clickhouse-reload-config

/etc/clickhouse-server/cluster.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_cluster_config
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - require:
            - test: clickhouse-server-cluster-req
        - watch_in:
            - test: clickhouse-server-cluster-ready

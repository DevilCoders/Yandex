clickhouse-server-users-req:
    test.nop

clickhouse-server-users-ready:
    test.nop:
        - on_changes_in:
            - mdb_clickhouse: clickhouse-reload-config

/etc/clickhouse-server/users.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_users_config
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - require:
            - test: clickhouse-server-users-req
        - watch_in:
            - test: clickhouse-server-users-ready

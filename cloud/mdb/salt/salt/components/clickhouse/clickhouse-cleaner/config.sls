ch-cleaner-config:
    fs.file_present:
        - name: /etc/yandex/clickhouse-cleaner/clickhouse-cleaner.conf
        - contents_function: mdb_clickhouse.cleaner_config
        - contents_format: yaml
        - mode: 644
        - makedirs: True

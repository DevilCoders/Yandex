do-backup:
    module.run:
        - name: mdb_elasticsearch.do_backup
    cmd.run:
        - name: /usr/local/yandex/es_backups.py update-meta --force >> /var/log/elasticsearch/backups.log 2>&1

do-initial-ch-backup:
    cmd.run:
        - name: {{ salt.mdb_clickhouse.initial_backup_command() }}
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8

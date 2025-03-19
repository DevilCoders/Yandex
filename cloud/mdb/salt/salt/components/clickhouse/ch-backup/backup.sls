do-ch-backup:
    cmd.run:
        - name: {{ salt.mdb_clickhouse.backup_command() }}
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8


{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-backup-req:
    test.nop

sqlserver-backup-ready:
    test.nop

databases-backuped:
    mdb_sqlserver.db_backuped:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - databases: {{ sqlserver.databases_list_full|tojson }}
        - backup_id: NEW
        - require:
            - test: sqlserver-backup-req
        - require_in:
            - test: sqlserver-backup-ready

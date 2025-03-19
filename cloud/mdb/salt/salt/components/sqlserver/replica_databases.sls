{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-databases-req:
    test.nop

sqlserver-databases-ready:
    test.nop


databases-restored:
    mdb_sqlserver.db_restored:
        - databases: {{ sqlserver.databases_list|tojson }}
{% if salt['pillar.get']('restore-from') %}
        - walg_config: 'C:\ProgramData\wal-g\wal-g-restore.yaml'
        - backup_id: {{ salt['pillar.get']('restore-from:backup-id', 'LATEST') }}
        - until_ts: {{ salt['pillar.get']('restore-from:time', '9999-12-31T23:59:59Z') }}
        - timeout: 0
{% elif salt['pillar.get']('db-restore-from') %}
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - backup_id: {{ salt['pillar.get']('db-restore-from:backup-id', 'LATEST') }}
        - from_databases:
            - {{ salt['pillar.get']('db-restore-from:database')|yaml_encode }}
        - until_ts: {{ salt['pillar.get']('db-restore-from:time', '9999-12-31T23:59:59Z') }}
        - timeout: 0
{% else %}
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - backup_id: LATEST
        - until_ts: '9999-12-31T23:59:59Z'
        - timeout: 600
{% endif %}
        - norecovery: True
        - roll_forward: True
        - require:
            - test: sqlserver-databases-req

wait-primary-dbs-in-ags:
    mdb_sqlserver.wait_all_dbs_listed_in_ag:
        - dbs: {{ sqlserver.databases_list|tojson }}
        - require:
            - mdb_sqlserver: databases-restored

secondary-dbs-in-ags:
    mdb_sqlserver.all_secondary_dbs_in_ags:
        - dbs: {{ sqlserver.databases_list|tojson }}
        - edition: {{ sqlserver.edition|yaml_encode }}
        - require:
            - mdb_sqlserver: wait-primary-dbs-in-ags
        - require_in:
            - test: sqlserver-databases-ready

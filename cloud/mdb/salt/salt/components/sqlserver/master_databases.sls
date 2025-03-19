{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-databases-req:
    test.nop:
        - require:
            - test: sqlserver-ag-ready

sqlserver-databases-created:
    test.nop

sqlserver-databases-ready:
    test.nop

{% if salt['pillar.get']('restore-from') %}

databases-restored:
    mdb_sqlserver.db_restored:
        - walg_config: 'C:\ProgramData\wal-g\wal-g-restore.yaml'
        - databases: {{ sqlserver.databases_list_full|tojson }}
        - backup_id: {{ salt['pillar.get']('restore-from:backup-id', 'LATEST') }}
        - until_ts: {{ salt['pillar.get']('restore-from:time', '9999-12-31T23:59:59Z') }}
        - timeout: 0
        - require:
            - test: sqlserver-ag-req
        - require_in:
            - test: sqlserver-databases-req

{% elif salt['pillar.get']('db-restore-from') %}

databases-restored:
    mdb_sqlserver.db_restored:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - databases: {{ sqlserver.databases_list|tojson }}
        - backup_id: {{ salt['pillar.get']('db-restore-from:backup-id', 'LATEST')|yaml_encode }}
        - from_databases:
            - {{ salt['pillar.get']('db-restore-from:database')|yaml_encode }}
        - until_ts: {{ salt['pillar.get']('db-restore-from:time', '9999-12-31T23:59:59Z')|yaml_encode }}
        - require:
          - test: sqlserver-databases-req
        - require_in:
          - test: sqlserver-databases-created
    
{% else %}

databases-present:
    mdb_sqlserver.dbs_present:
        - databases: {{ sqlserver.databases_list|tojson }}
        - require:
          - test: sqlserver-databases-req
        - require_in:
          - test: sqlserver-databases-created
          - test: sqlserver-databases-ready

databases-recovered:
    mdb_sqlserver.db_recovered:
        - databases: {{ sqlserver.databases_list_full|tojson }}
        - require:
            - test: sqlserver-ag-req
        - require_in:
            - test: sqlserver-databases-req
        - onchanges:
            - mdb_sqlserver: ags-present

{% endif %}


{% if salt['pillar.get']('do-backup') %}
{# do full backup for new or restored cluster #}

include:
    - .run-backup

extend:
    databases-backuped:
        mdb_sqlserver.db_backuped:
            - require:
                - test: sqlserver-databases-created
{% if salt['pillar.get']('restore-from:cid') %}
                - mdb_sqlserver: wait-replica-dbs-joined 
{% else %}
            - require_in:
                - mdb_sqlserver: wait-replica-dbs-joined 
{% endif %}

{% elif salt['pillar.get']('backup-import') %}
{# backup only logs for imported database #}

databases-backuped:
    mdb_sqlserver.db_log_backuped:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - databases: {{ sqlserver.databases_list|tojson }}
        - require:
            - test: sqlserver-databases-created
        - require_in:
            - mdb_sqlserver: wait-replica-dbs-joined 

{% elif salt['pillar.get']('target-database') %}
{# backup one database while creating a new one #}

databases-backuped: 
    mdb_sqlserver.db_backuped:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - databases: {{ sqlserver.databases_list|tojson }}
        - backup_id: LATEST
        - require:
            - test: sqlserver-databases-created
{% if salt['pillar.get']('db-restore-from') %}
            - mdb_sqlserver: wait-replica-dbs-joined 
{% else %}
        - require_in:
            - mdb_sqlserver: wait-replica-dbs-joined 
{% endif %}
{% endif %}

dbs-in-ags:
    mdb_sqlserver.dbs_in_ags:
        - databases: {{ sqlserver.db_ag_dict|tojson }}
        - require:
            - test: sqlserver-databases-created
        - require_in:
            - test: sqlserver-databases-ready

wait-replica-dbs-joined:
    mdb_sqlserver.wait_secondary_database_joined:
        - nodes: {{ sqlserver.db_hosts|tojson }}
        - databases: {{ sqlserver.databases_list|tojson }}
        - timeout: 300
        - sleep: 5
        - require:
            - test: sqlserver-databases-ready
            - mdb_sqlserver: replicas-present

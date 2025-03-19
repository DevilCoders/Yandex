{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-ag-req:
    test.nop

sqlserver-ag-ready:
    test.nop

ags-present:
    mdb_sqlserver.all_ags_present:
        - ags: {{ sqlserver.ags_list|tojson }}
        - basic: {{ sqlserver.basic_ag }}
        - host: {{ salt['pillar.get']('data:dbaas:fqdn', salt['mdb_windows.get_fqdn']()) }}
        - require: 
            - test: sqlserver-ag-req
        - require_in:
            - test: sqlserver-ag-ready

wait-ags-online:
    mdb_windows.wait_cluster_resources_up:
        - resources: {{ sqlserver.ags_list|tojson }}
        - timeout: 60
        - interval: 10
        - require:
            - test: sqlserver-ag-req
            - mdb_sqlserver: ags-present
        - require_in:
            - test: sqlserver-ag-ready

replicas-present:
    mdb_sqlserver.all_replicas_present:
        - ags: {{ sqlserver.ags_list|tojson }}
        - nodes: {{ sqlserver.db_hosts|tojson }}
        - availability_mode: 'SYNCHRONOUS_COMMIT'
{% if salt['pillar.get']('data:sqlserver:unreadable_replicas', false) == true %}
        - secondary_allow_connections: 'NO'
{% else %}
        - secondary_allow_connections: 'ALL'
{% endif %}
        - primary_allow_connections: 'ALL'
        - seeding_mode: 'MANUAL'
        - require:
            - test: sqlserver-ag-req
            - mdb_windows: wait-ags-online
        - require_in:
            - test: sqlserver-ag-ready

replicas-failover-enabled:
    mdb_sqlserver.all_replicas_present:
        - ags: {{ sqlserver.ags_list|tojson }}
        - nodes: {{ sqlserver.db_hosts|tojson }}
        - failover_mode: 'AUTOMATIC'
        - require:
            - mdb_sqlserver: wait-replica-dbs-joined

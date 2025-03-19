{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-ag-req:
    test.nop

sqlserver-ag-ready:
    test.nop

ag-joined:
    mdb_sqlserver.join_ags:
        - ags: {{ sqlserver.ags_list|tojson }}
        - timeout: 600
        - sleep: 10 
        - require:
            - test: sqlserver-ag-req
        - require_in:
            - test: sqlserver-ag-ready

replicas-connected:
    mdb_sqlserver.wait_all_replicas_connected:
        - dbs: {{ sqlserver.databases_list|tojson }}
        - edition: {{ sqlserver.edition|yaml_encode }}
        - timeout: 300
        - interval: 5
        - onchanges:
            - mdb_sqlserver: ag-joined
        - require_in:
            - test: sqlserver-ag-ready


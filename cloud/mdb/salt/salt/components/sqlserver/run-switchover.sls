{% from "components/sqlserver/map.jinja" import sqlserver with context %}

sqlserver-switchover-req:
    test.nop

sqlserver-pre-switchover:
    mdb_sqlserver.wait_synchronized:
      - timeout_ms: 20000
      - require:
          - test: sqlserver-switchover-req

sqlserver-switchover:
    mdb_sqlserver.ensure_master:
      - ag: {{ sqlserver.ags_list|tojson }}
      - require:
          - mdb_sqlserver: sqlserver-pre-switchover
      - require_in:
          - test: sqlserver-switchover-ready

sqlserver-switchover-ready:
    test.nop

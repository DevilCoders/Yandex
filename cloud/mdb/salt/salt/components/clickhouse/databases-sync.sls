{% set dbaas = salt.pillar.get('data:dbaas', {}) %}

clickhouse-sync-databases-req:
    test.nop

clickhouse-sync-databases-ready:
    test.nop


sync_databases:
    mdb_clickhouse.sync_databases:
        - require:
            - test: clickhouse-sync-databases-req
        - require_in:
            - test: clickhouse-sync-databases-ready

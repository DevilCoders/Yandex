sync-replication-slots-req:
    test.nop

sync_replication_slots:
    mdb_postgresql.sync_physical_replication_slots:
        - require:
            - test: sync-replication-slots-req

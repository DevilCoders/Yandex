mongos-sync-check-collections-req:
    test.nop

mongos-sync-check-collections-done:
    test.nop

ensure-check-collections:
    mdb_mongodb.ensure_check_collections:
      - db: mdb_internal
      - prefix: check_
      - service: mongos
      - require:
        - test: mongos-sync-check-collections-req
      - require_in:
        - test: mongos-sync-check-collections-done

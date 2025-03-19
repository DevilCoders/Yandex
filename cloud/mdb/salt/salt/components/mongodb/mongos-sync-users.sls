sync_mongos_users:
    mdb_mongodb.ensure_users:
        - service: mongos

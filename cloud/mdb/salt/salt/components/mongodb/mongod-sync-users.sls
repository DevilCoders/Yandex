sync_mongod_users:
    mdb_mongodb.ensure_users:
        - service: mongod

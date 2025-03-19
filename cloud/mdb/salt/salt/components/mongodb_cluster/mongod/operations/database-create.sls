include:
    - components.mongodb.mongod-sync-roles
    - components.mongodb.mongod-sync-users
    - components.mongodb.mongod-sync-databases

extend:
    sync_mongod_users:
        mdb_mongodb.ensure_users:
            - require:
                - sync_mongod_roles

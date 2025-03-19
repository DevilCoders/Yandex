include:
    - components.mongodb.mongod-sync-roles
    - components.mongodb.mongod-sync-users
    - components.mongodb.mongod-delete-databases

extend:
    sync_mongod_roles:
        mdb_mongodb.ensure_roles:
            - require:
                - sync_mongod_users

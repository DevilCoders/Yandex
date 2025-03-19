include:
    - components.mongodb.mongos-sync-roles
    - components.mongodb.mongos-sync-users
    - components.mongodb.mongos-sync-databases

extend:
    sync_mongos_users:
        mdb_mongodb.ensure_users:
            - require:
                - sync_mongos_roles

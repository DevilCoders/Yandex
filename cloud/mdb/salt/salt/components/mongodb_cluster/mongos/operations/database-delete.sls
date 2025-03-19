include:
    - components.mongodb.mongos-sync-roles
    - components.mongodb.mongos-sync-users
    - components.mongodb.mongos-delete-databases

extend:
    sync_mongos_roles:
        mdb_mongodb.ensure_roles:
            - require:
                - sync_mongos_users

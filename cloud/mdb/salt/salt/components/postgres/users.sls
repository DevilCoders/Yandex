pg_sync_users:
    mdb_postgresql.sync_users

pg_sync_user_grants:
    mdb_postgresql.sync_user_grants:
        - require:
            - mdb_postgresql: pg_sync_users

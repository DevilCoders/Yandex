{% from "components/postgres/pg.jinja" import pg with context %}

sync_users_test:
    mdb_postgresql.sync_users

sync_user_grants_test:
    mdb_postgresql.sync_user_grants:
        - require:
            - mdb_postgresql: sync_users_test

sync_databases_test:
    mdb_postgresql.sync_databases:
        - require:
            - mdb_postgresql: sync_users_test

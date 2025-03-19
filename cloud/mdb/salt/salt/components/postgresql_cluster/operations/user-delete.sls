{% from "components/postgres/pg.jinja" import pg with context %}
include:
{% if pg.connection_pooler == 'odyssey' %}
    - components.postgres.odyssey
{% else %}
    - components.postgres.configs.pgbouncer-userlist
{% endif %}
    - components.postgres.configs.pg_hba
    - components.postgres.service
    - components.postgres.users
    - components.pg-dbs.unmanaged.sync-databases

extend:
    pg_sync_databases:
        mdb_postgresql.sync_databases:
            - require:
                - mdb_postgresql: pg_sync_users

    postgresql-service:
        service.running:
            - watch:
                - file: {{ pg.data }}/conf.d/pg_hba.conf

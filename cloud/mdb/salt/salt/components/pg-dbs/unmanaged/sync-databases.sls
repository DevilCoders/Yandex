{% from "components/postgres/pg.jinja" import pg with context %}

pg_sync_databases:
    mdb_postgresql.sync_databases

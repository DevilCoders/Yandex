BEGIN;

CREATE EXTENSION IF NOT EXISTS postgres_fdw;
DROP SERVER IF EXISTS postgresdb CASCADE;
CREATE SERVER postgresdb FOREIGN DATA WRAPPER postgres_fdw
    OPTIONS (dbname 'postgres', host 'localhost', port '{{ salt['pillar.get']('data:config:postgres_fdw_port', '6432')}}', updatable 'false');

CREATE USER MAPPING FOR PUBLIC SERVER postgresdb
    OPTIONS (user 'monitor', password '{{ salt['pillar.get']('data:config:pgusers:monitor:password', '') }}');
ALTER USER MAPPING FOR PUBLIC SERVER postgresdb
    OPTIONS (set user 'monitor', set password '{{ salt['pillar.get']('data:config:pgusers:monitor:password', '') }}');

CREATE FOREIGN TABLE IF NOT EXISTS public.repl_mon
    (
      ts timestamp with time zone,
      location text,
      replics integer,
      master text
    )
    SERVER postgresdb
        OPTIONS (table_name 'repl_mon',schema_name 'public');

GRANT SELECT ON repl_mon to PUBLIC;

COMMIT;

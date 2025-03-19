CREATE EXTENSION IF NOT EXISTS dblink;
DROP SCHEMA IF EXISTS pgbouncer CASCADE;
CREATE SCHEMA IF NOT EXISTS pgbouncer;

DO $$DECLARE 
i int;
t text;
BEGIN
    FOR i IN 0..{{ salt['pillar.get']('data:pgbouncer:internal_count', 1)-1 }}  LOOP
        EXECUTE 'DROP SERVER IF EXISTS pgbouncer_internal'||trim(to_char(i,'09'))||' CASCADE';

        EXECUTE 'CREATE SERVER pgbouncer_internal'||trim(to_char(i,'09'))||' FOREIGN DATA WRAPPER dblink_fdw 
        OPTIONS (host ''/var/run/pgbouncer_internal'||trim(to_char(i,'09'))||''', port ''7432'', dbname ''pgbouncer'') ';

        EXECUTE 'CREATE USER MAPPING FOR PUBLIC SERVER pgbouncer_internal'|| trim(to_char(i,'09')) ||' 
        OPTIONS (user ''monitor'', password ''{{ salt['pillar.get']('data:config:pgusers:monitor:password', '') }}'')';

        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.clients_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show clients'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.config_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show config'') AS
                     _(
                         key text,
                         value text,
                         changeable boolean
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.databases_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show databases'') AS
                     _(
                         name text,
                         host text,
                         port int,
                         database text,
                         force_user text,
                         pool_size int,
                         reserve_pool int,
                         pool_mode text,
                         max_connections int,
                         current_connections int,
                         paused int,
                         disabled int
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.lists_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show lists'') AS
                     _(
                         list text,
                         items int
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.pools_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show pools'') AS
                     _(
                         database text,
                         "user" text,
                         cl_active int,
                         cl_waiting int,
                         sv_active int,
                         sv_idle int,
                         sv_used int,
                         sv_tested int,
                         sv_login int,
                         maxwait int,
                         maxwait_us int,
                         pool_mode text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.servers_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show servers'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.sockets_internal'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer_internal'||trim(to_char(i,'09'))||''', ''show sockets'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text,
                         recv_pos int,
                         pkt_pos int,
                         pkt_remain int,
                         send_pos int,
                         send_remain int,
                         pkt_avail int,
                         send_avail int
                     )';

    END LOOP;
END$$;


DO $$DECLARE 
i int;
t text;
BEGIN
    FOR i IN 0..{{ salt['pillar.get']('data:pgbouncer:count', 1)-1 }} LOOP
        EXECUTE 'DROP SERVER IF EXISTS pgbouncer'||trim(to_char(i,'09'))||' CASCADE';

        EXECUTE 'CREATE SERVER pgbouncer'||trim(to_char(i,'09'))||' FOREIGN DATA WRAPPER dblink_fdw 
        OPTIONS (host ''/var/run/pgbouncer'||trim(to_char(i,'09'))||''', port ''6432'', dbname ''pgbouncer'') ';

        EXECUTE 'CREATE USER MAPPING FOR PUBLIC SERVER pgbouncer'|| trim(to_char(i,'09')) ||' 
        OPTIONS (user ''monitor'', password ''{{ salt['pillar.get']('data:config:pgusers:monitor:password', '') }}'')';

        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.clients_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show clients'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.config_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show config'') AS
                     _(
                         key text,
                         value text,
                         changeable boolean
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.databases_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show databases'') AS
                     _(
                         name text,
                         host text,
                         port int,
                         database text,
                         force_user text,
                         pool_size int,
                         reserve_pool int,
                         pool_mode text,
                         max_connections int,
                         current_connections int,
                         paused int,
                         disabled int
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.lists_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show lists'') AS
                     _(
                         list text,
                         items int
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.pools_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show pools'') AS
                     _(
                         database text,
                         "user" text,
                         cl_active int,
                         cl_waiting int,
                         sv_active int,
                         sv_idle int,
                         sv_used int,
                         sv_tested int,
                         sv_login int,
                         maxwait int,
                         maxwait_us int,
                         pool_mode text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.servers_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show servers'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text
                     )';
        EXECUTE 'CREATE OR REPLACE VIEW pgbouncer.sockets_external'||trim(to_char(i,'09'))||' AS
                     SELECT * FROM dblink(''pgbouncer'||trim(to_char(i,'09'))||''', ''show sockets'') AS
                     _(
                         type text,
                         "user" text,
                         database text,
                         state text,
                         addr text,
                         port int,
                         local_addr text,
                         local_port int,
                         connect_time timestamp with time zone,
                         request_time timestamp with time zone,
                         wait int,
                         wait_us int,
                         ptr text,
                         link text,
                         remote_pid int,
                         tls text,
                         recv_pos int,
                         pkt_pos int,
                         pkt_remain int,
                         send_pos int,
                         send_remain int,
                         pkt_avail int,
                         send_avail int
                     )';

    END LOOP;
END$$;



GRANT USAGE ON SCHEMA pgbouncer TO monitor;
GRANT SELECT ON ALL TABLES IN SCHEMA pgbouncer TO monitor;

    DO $$DECLARE
     i text;
     sql text;
     x text;
     t int:=0;
     tabels text[]:= '{sockets,clients,config,databases,lists,pools,servers}';
    BEGIN
      FOREACH x in ARRAY tabels LOOP 
      t:=0;
      sql:='';
        FOR i IN (select viewname from pg_views where schemaname ='pgbouncer' and viewname ~ x and viewname!=x) LOOP
         IF t=0 THEN
          sql:='create or replace view pgbouncer.'|| x ||' AS select '''||replace(i,x||'_','') ||''' AS bouncer,* from pgbouncer.'|| i ;
          ELSE
           sql:=sql||' UNION ALL select '''||replace(i,x||'_','') ||''' AS bouncer,* FROM pgbouncer.'|| i ;
         END IF;
          t:=t+1;
        END LOOP;
           EXECUTE sql;
       END LOOP;
    END$$;

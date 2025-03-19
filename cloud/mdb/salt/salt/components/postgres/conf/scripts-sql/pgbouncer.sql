CREATE EXTENSION IF NOT EXISTS dblink;
DROP SERVER IF EXISTS pgbouncer CASCADE;
CREATE SERVER IF NOT EXISTS pgbouncer FOREIGN DATA WRAPPER dblink_fdw OPTIONS (host 'localhost',
                                                                 port '6432',
                                                                 dbname 'pgbouncer');

CREATE USER MAPPING FOR PUBLIC SERVER pgbouncer
    OPTIONS (user 'monitor', password '{{ salt['pillar.get']('data:config:pgusers:monitor:password', '') }}');

DROP SCHEMA IF EXISTS pgbouncer CASCADE;
CREATE SCHEMA IF NOT EXISTS pgbouncer;

CREATE OR REPLACE VIEW pgbouncer.clients AS
    SELECT * FROM dblink('pgbouncer', 'show clients') AS
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
    );

CREATE OR REPLACE VIEW pgbouncer.config AS
    SELECT * FROM dblink('pgbouncer', 'show config') AS
    _(
        key text,
        value text,
        changeable boolean
    );

CREATE OR REPLACE VIEW pgbouncer.databases AS
    SELECT * FROM dblink('pgbouncer', 'show databases') AS
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
    );

CREATE OR REPLACE VIEW pgbouncer.lists AS
    SELECT * FROM dblink('pgbouncer', 'show lists') AS
    _(
        list text,
        items int
    );

CREATE OR REPLACE VIEW pgbouncer.pools AS
    SELECT * FROM dblink('pgbouncer', 'show pools') AS
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
    );

CREATE OR REPLACE VIEW pgbouncer.servers AS
    SELECT * FROM dblink('pgbouncer', 'show servers') AS
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
    );

CREATE OR REPLACE VIEW pgbouncer.sockets AS
    SELECT * FROM dblink('pgbouncer', 'show sockets') AS
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
    );

CREATE OR REPLACE VIEW pgbouncer.errors AS
    SELECT * FROM dblink('pgbouncer', 'show errors') AS
    _(
        type text,
        cnt int
    );

CREATE OR REPLACE VIEW pgbouncer.error_per_route AS
    SELECT * FROM dblink('pgbouncer', 'show errors_per_route') AS
    _(
        type text,
        usr text,
        db text,
        cnt int
    );

GRANT USAGE ON SCHEMA pgbouncer TO monitor;
GRANT USAGE ON FOREIGN SERVER pgbouncer TO monitor;
GRANT SELECT ON ALL TABLES IN SCHEMA pgbouncer TO monitor;

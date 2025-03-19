CREATE OR REPLACE FUNCTION plproxy.update_remote_tables()
 RETURNS integer
 LANGUAGE plpgsql
AS $function$
BEGIN
    create extension if not exists postgres_fdw;
--start of the section for FDW

--end of the section for FDW

    create foreign table parts
        (
          part_id integer not null
        ) server remote;
    begin
    drop table if exists plproxy.parts;
    create table plproxy.parts (like parts);
    insert into plproxy.parts select * from parts;
    end;

    create foreign table hosts
        (
          host_id integer not null,
          host_name varchar(100) not null,
          dc varchar(10),
          base_prio smallint null,
          prio_diff smallint null
        ) server remote;
    begin
    drop table if exists plproxy.hosts;
    create table plproxy.hosts (like hosts);
    insert into plproxy.hosts select * from hosts;
    end;

    create foreign table connections
        (
          conn_id integer not null,
          conn_string varchar(255) not null
        ) server remote;
    begin
    drop table if exists plproxy.connections;
    create table plproxy.connections (like connections);
    insert into plproxy.connections select * from connections;
    end;

    create foreign table priorities
        (
          conn_id integer not null,
          host_id integer not null,
          part_id integer not null,
          priority smallint not null
        ) server remote;
    begin
    drop table if exists plproxy.priorities;
    create table plproxy.priorities (like priorities);
    create trigger update_cluster_version AFTER INSERT or DELETE or UPDATE on plproxy.priorities for each statement execute procedure plproxy.inc_cluster_version();
    insert into plproxy.priorities select * from priorities;
    end;

    create foreign table config
        (
          key varchar(50) not null,
          value varchar(255)
        ) server remote;
    begin
    drop table if exists plproxy.config;
    create table plproxy.config (like config);
    insert into plproxy.config select * from config;
    end;

    create foreign table key_ranges
        (
          range_id integer not null,
          part_id integer not null,
          start_key bigint not null,
          end_key bigint not null
        ) server remote;
    begin
    drop table if exists plproxy.key_ranges;
    create table plproxy.key_ranges (like key_ranges);
    insert into plproxy.key_ranges select * from key_ranges;
    end;

--start of the section for grants

--end of the section for grants

    update plproxy.versions set version = version + 1;

    drop server remote cascade;
    return 0;
END;
$function$

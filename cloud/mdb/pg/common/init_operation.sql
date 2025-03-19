CREATE OR REPLACE FUNCTION public.init()
 RETURNS int
 LANGUAGE plpgsql
 -- common options:  IMMUTABLE  STABLE  STRICT  SECURITY DEFINER
AS $function$
declare
    new_part integer;
    start_key bigint;
    range_size bigint;
    new_range integer;
    tmp_host_id integer;
    tmp_conn_id integer;
    tmp text;
    host_is_found boolean;
    dbname text;
    i_hostname text;
begin
    range_size:={{ salt['pillar.get']('data:config:range_size', '9223372036854775807-1') }};
    i_hostname:='{{ salt['grains.get']('id') }}';
    select current_database() into dbname;

    create extension if not exists postgres_fdw;
    {% set host = salt['pillar.get']('data:pgmeta:server', 'pgmeta.mail.yandex.net') %}
    {% set dbname = salt['pillar.get']('data:pgmeta:dbname', default_dbname) %}
    {% set port = salt['pillar.get']('data:pgmeta:write_port', '6432') %}
    create server remote foreign data wrapper postgres_fdw options (host '{{ host }}', dbname '{{ dbname }}', port '{{ port }}');
    create user mapping for postgres server remote options (user 'pgproxy', password '{{ salt['pillar.get']('data:config:pgusers:pgproxy:password', '') }}');
    create foreign table key_ranges
            (
          range_id integer not null,
              part_id integer not null,
              start_key bigint not null,
              end_key bigint not null
            ) server remote;
    create foreign table parts
        (
          part_id integer not null
        ) server remote;
    create foreign table hosts
        (
          host_id integer not null,
              host_name varchar(100) not null,
          dc varchar(10),
          base_prio smallint null,
              prio_diff smallint null
        ) server remote;
    create foreign table connections
        (
          conn_id integer not null,
              conn_string varchar(255) not null
        ) server remote;
    create foreign table priorities
            (
              part_id integer not null,
              host_id integer not null,
              conn_id integer not null,
              priority smallint default 100 not null
            ) server remote;

    begin
    select max(part_id) into new_part from parts;
    if (new_part is null) then
            new_part:=0;
    else
            new_part:=new_part+1;
    end if;
    insert into parts values(new_part);
    end;

    begin
    select max(range_id)+1 into new_range from key_ranges;
    if (new_range is null) then
        new_range:=1;
    end if;
    select max(end_key) into start_key from key_ranges;
    if (start_key is null) then
        start_key:=1;
    else
        start_key:=start_key+1;
    end if;
    insert into key_ranges values (new_range, new_part, start_key, start_key+range_size);
    end;

    begin
        select host_id into tmp_host_id from hosts where host_name=i_hostname;
        if (tmp_host_id is null) then
            select max(host_id)+1 into tmp_host_id from hosts;
            if (tmp_host_id is null) then
                tmp_host_id:=1;
            end if;
            insert into hosts values (tmp_host_id, i_hostname);

        else
            host_is_found:=true;
        end if;

        select max(conn_id)+1 into tmp_conn_id from connections;
        if (tmp_conn_id is null) then
            tmp_conn_id:=1;
        end if;
        insert into connections values (tmp_conn_id, 'host=' || i_hostname || ' port=6432 dbname=' || dbname);
        insert into priorities (part_id, host_id, conn_id, priority) values (new_part, tmp_host_id, tmp_conn_id, 0);

        for tmp in
            select client_hostname from pg_stat_replication
        loop
            if host_is_found then
                select host_id into tmp_host_id from hosts where host_name=tmp;
            else
                select max(host_id)+1 into tmp_host_id from hosts;
                insert into hosts values(tmp_host_id, tmp);
            end if;
            tmp_conn_id:=tmp_conn_id+1;
            insert into connections values(tmp_conn_id, 'host=' || tmp || ' port=6432 dbname=' || dbname);
            insert into priorities (part_id, host_id, conn_id, priority) values (new_part, tmp_host_id, tmp_conn_id, 10);
        end loop;
    end;

    drop server remote cascade;

    return 0;
end;
$function$

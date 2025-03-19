--create table clusters
--  (
--    cluster_id integer not null,
--    cluster_name varchar(50) not null,
--    constraint PK_CLUSTERS primary key (cluster_id)
--  );

create table parts
    (
      part_id integer not null,
--    cluster_id integer not null references clusters (cluster_id) on delete cascade,
      constraint PK_PARTS primary key (part_id)
    );

create table hosts
    (
      host_id integer not null,
      host_name varchar(100) not null,
      dc varchar(10),
      base_prio smallint null,
          prio_diff smallint null,
      constraint PK_HOSTS primary key (host_id)
    );

create table connections
    (
      conn_id integer not null,
      conn_string varchar(255) not null,
      constraint PK_CONNECTIONS primary key (conn_id)
    );

create table config
    (
--    cluster_id integer not null references clusters (cluster_id) on delete cascade,
      key varchar(50) not null,
      value varchar(255),
      constraint UK_CONFIG_CLUSTER_KEY unique (key) --unique (cluster_id, key)
    );

create table priorities
        (
          part_id integer not null,
          host_id integer not null,
          conn_id integer not null,
          priority smallint default 100 not null
        );

--create sequence KEY_RANGES_ID_SEQ start with 1 increment by 1 no maxvalue;

create table key_ranges
    (
      range_id integer not null, --default nextval('key_ranges_id_seq'),
          part_id integer not null references parts (part_id) on delete cascade,
          start_key bigint not null,
          end_key bigint not null,
      constraint PK_KEY_RANGES primary key (range_id)
    );

GRANT USAGE ON SCHEMA public TO monitor;
GRANT INSERT, UPDATE, SELECT ON ALL TABLES IN SCHEMA public TO pgproxy;

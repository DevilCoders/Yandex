create schema if not exists plproxy;

-- handler function
CREATE OR REPLACE FUNCTION plproxy_call_handler ()
RETURNS language_handler AS 'plproxy' LANGUAGE C;

-- language
CREATE OR REPLACE LANGUAGE plproxy HANDLER plproxy_call_handler;

-- validator function
CREATE OR REPLACE FUNCTION plproxy_fdw_validator (text[], oid)
RETURNS boolean AS 'plproxy' LANGUAGE C;

-- foreign data wrapper
DROP FOREIGN DATA WRAPPER IF EXISTS plproxy CASCADE;
CREATE FOREIGN DATA WRAPPER plproxy VALIDATOR plproxy_fdw_validator;

create table plproxy.parts
	(
	  part_id integer not null
	);

create table plproxy.hosts
	(
	  host_id integer not null,
	  host_name varchar(100) not null,
	  dc varchar(10)
	);

create table plproxy.connections
	(
	  conn_id integer not null,
	  conn_string varchar(255) not null
	);

create table plproxy.versions
	(
	  version bigint default 0 not null
	);

insert into plproxy.versions values (1);

create table plproxy.priorities
	(
	  part_id integer not null,
	  host_id integer not null,
	  conn_id integer not null,
	  priority smallint default 100 not null
	);

create table plproxy.config
	(
	  key varchar(50) not null,
	  value varchar(255)
	);

create table plproxy.key_ranges
	(
	  range_id integer not null,
	  part_id integer not null,
	  start_key bigint not null,
	  end_key bigint not null
	);

GRANT USAGE ON SCHEMA plproxy TO monitor;
GRANT ALL ON ALL TABLES IN SCHEMA plproxy TO monitor;

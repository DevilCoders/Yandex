{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set admin_role = 'mdb_admin' %}
{% set schema = 'mdb_toolkit' %}

DO $$
BEGIN
  CREATE ROLE {{ admin_role }} WITH NOLOGIN;
  EXCEPTION WHEN DUPLICATE_OBJECT THEN
  RAISE NOTICE 'not creating role {{ admin_role }} -- it already exists';
END
$$;

create schema if not exists {{ schema }};

revoke all on schema {{ schema }} from public;
revoke all on schema {{ schema }} from {{ admin_role }};

grant all on schema {{ schema }} to {{ gpdbvars.gpadmin }};
grant usage on schema {{ schema }} to {{ admin_role }};

CREATE OR REPLACE FUNCTION {{ schema }}.pg_stat_activity() RETURNS SETOF pg_stat_activity AS
$$ SELECT * FROM pg_catalog.pg_stat_activity; $$
LANGUAGE sql CONTAINS SQL SECURITY DEFINER;

REVOKE ALL ON FUNCTION {{ schema }}.pg_stat_activity() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION {{ schema }}.pg_stat_activity() TO {{ admin_role }}; 

CREATE OR REPLACE FUNCTION {{ schema }}.pg_locks() RETURNS SETOF pg_locks AS
$$ SELECT * FROM pg_catalog.pg_locks; $$
LANGUAGE sql CONTAINS SQL SECURITY DEFINER;

REVOKE ALL ON FUNCTION {{ schema }}.pg_locks() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION {{ schema }}.pg_locks() TO {{ admin_role }};

CREATE OR REPLACE FUNCTION {{ schema }}.session_level_memory_consumption() RETURNS SETOF session_state.session_level_memory_consumption AS
$$ SELECT * FROM session_state.session_level_memory_consumption; $$
LANGUAGE sql CONTAINS SQL SECURITY DEFINER;

REVOKE ALL ON FUNCTION {{ schema }}.session_level_memory_consumption() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION {{ schema }}.session_level_memory_consumption() TO {{ admin_role }};

CREATE OR REPLACE FUNCTION {{ schema }}.can_write() RETURNS BOOLEAN AS $$
BEGIN
    INSERT INTO {{ schema }}.write_check SELECT generate_series(1,1000);
    TRUNCATE {{ schema }}.write_check;
    RETURN TRUE;
END;
$$
STRICT
LANGUAGE plpgsql;
REVOKE ALL ON FUNCTION {{ schema }}.can_write() FROM PUBLIC;

DROP TABLE IF EXISTS {{ schema }}.write_check;
CREATE TABLE {{ schema }}.write_check (id int) DISTRIBUTED BY (id);
REVOKE ALL ON TABLE {{ schema }}.write_check FROM PUBLIC;

CREATE OR REPLACE FUNCTION {{ schema }}.billing_can_write() RETURNS BOOLEAN AS $$
BEGIN
    INSERT INTO {{ schema }}.billing_write_check SELECT generate_series(1,1000);
    TRUNCATE {{ schema }}.billing_write_check;
    RETURN TRUE;
END;
$$
STRICT
LANGUAGE plpgsql;
REVOKE ALL ON FUNCTION {{ schema }}.billing_can_write() FROM PUBLIC;

DROP TABLE IF EXISTS {{ schema }}.billing_write_check;
CREATE TABLE {{ schema }}.billing_write_check (id int) DISTRIBUTED BY (id);
REVOKE ALL ON TABLE {{ schema }}.billing_write_check FROM PUBLIC;


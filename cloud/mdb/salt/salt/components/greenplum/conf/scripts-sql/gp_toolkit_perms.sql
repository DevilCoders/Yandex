{% set mdb_admin_role = 'mdb_admin' %}
{% set pg_aoseg_schema = 'pg_aoseg' %}
{% set gp_toolkit_schema = 'gp_toolkit' %}
{% set gp_toolkit_tables = ['gp_size_of_table_uncompressed'] %}

DO $$
BEGIN
  CREATE ROLE {{ mdb_admin_role }} WITH NOLOGIN;
  EXCEPTION WHEN DUPLICATE_OBJECT THEN
  RAISE NOTICE 'not creating role {{ mdb_admin_role }} -- it already exists';
END
$$;

GRANT USAGE ON SCHEMA {{ pg_aoseg_schema }} to {{ mdb_admin_role }};


{% for table in gp_toolkit_tables %}
GRANT SELECT ON TABLE {{ gp_toolkit_schema }}.{{ table }} TO {{ mdb_admin_role }};
{% endfor %}

{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set admin_role = 'mdb_admin' %}
{% set schema = 'mdb_toolkit' %}

DROP TABLE if EXISTS {{ schema }}.db_daily_operations_log;
CREATE TABLE {{ schema }}.db_daily_operations_log
(
   database_name    text,
   schema_name      text,
   table_name       text,
   operation        text,
   status           text,
   start_time       timestamp,
   finish_time      timestamp
)
with (appendonly=true, compresstype=zstd, compresslevel=1) 
DISTRIBUTED RANDOMLY;

grant select on {{ schema }}.db_daily_operations_log to {{ admin_role }};



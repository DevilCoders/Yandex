{% from "components/sqlserver/map.jinja" import sqlserver with context %}
include:
{% if not sqlserver.is_replica %}
  - components.sqlserver.master
  - components.sqlserver.master_databases
{% else %}
  - components.sqlserver.replica
  - components.sqlserver.replica_databases
{% endif %}

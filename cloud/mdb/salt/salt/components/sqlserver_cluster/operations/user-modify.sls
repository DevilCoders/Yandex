{% from "components/sqlserver/map.jinja" import sqlserver with context %}
include:
  - components.sqlserver.logins
{% if not sqlserver.is_replica %}
  - components.sqlserver.users
{% endif %}

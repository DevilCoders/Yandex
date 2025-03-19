{% from "components/sqlserver/map.jinja" import sqlserver with context %}

include:
  - components.sqlserver.login-delete
{% if not sqlserver.is_replica %}
  - components.sqlserver.user-delete
{% endif %}

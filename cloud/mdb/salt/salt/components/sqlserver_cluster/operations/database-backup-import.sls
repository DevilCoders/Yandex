{% from "components/sqlserver/map.jinja" import sqlserver with context %}

{% if not sqlserver.is_replica %}
include:
    - components.sqlserver.database-backup-import
{% else %}
state-placeholder:
    test.nop
{% endif %}

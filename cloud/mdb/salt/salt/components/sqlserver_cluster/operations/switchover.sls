{% from "components/sqlserver/map.jinja" import sqlserver with context %}

{% if salt['pillar.get']('switchover:target_host') == salt['pillar.get']('data:dbaas:fqdn') %}
include:
   - components.sqlserver.run-switchover
{% else %}
state-placeholder:
    test.nop
{% endif %}


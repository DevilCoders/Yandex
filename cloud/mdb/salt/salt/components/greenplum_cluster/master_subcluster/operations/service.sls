include:
  - components.greenplum.configs.postgresql-conf-no-deps
  - components.greenplum.odyssey
{% if salt['pillar.get']('service-restart') %}
  - components.greenplum.restart
{% endif %}
{% if salt['pillar.get']('sync-passwords') %}
  - components.greenplum.password
{% endif %}

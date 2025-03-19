include:
    - .postgresql-conf
{% if salt['pillar.get']('data:greenplum:config:gpperfmon:enable', False) %}
    - .gpperfmon
{% endif %}
    - .pxf_datasource

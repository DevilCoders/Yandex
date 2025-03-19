sqlserver-users-req:
    test.nop

sqlserver-users-ready:
    test.nop

{% set filter = salt['pillar.get']('target-user') %}
{% for name, props in salt['pillar.get']('data:sqlserver:users', {}).items() %}
{%     if (not filter) or (filter and name == filter) %}


{% for db in salt['pillar.get']('data:sqlserver:databases', {}) %}
{% set dbprops = props.get('dbs', {}).get(db) %}
{% if dbprops is not none %}

create-user-{{ name|yaml_encode }}-{{ db|yaml_encode }}:
  mdb_sqlserver_user.present:
    - name: {{ name|yaml_encode }}
    - login: {{ name|yaml_encode }}
    - database: {{ db|yaml_encode }}
    - dbroles: {{ dbprops.get('roles', [])|tojson }}
    - require:
      - test: sqlserver-users-req
    - require_in:
      - test: sqlserver-users-ready
{% endif %}
{% endfor %}

{% endif %}
{% endfor %}

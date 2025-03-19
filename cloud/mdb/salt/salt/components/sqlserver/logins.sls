sqlserver-logins-req:
    test.nop

sqlserver-logins-ready:
    test.nop

mdb-monitor-present:
    mdb_sqlserver.server_role_present:
        - name: mdb_monitor
        - permissions:
            - 'VIEW SERVER STATE'
        - require_in:
            - test: sqlserver-logins-req

{% set filter = salt['pillar.get']('target-user') %}

{% for name, props in salt['pillar.get']('data:sqlserver:users', {}).items() %}
{%     if (not filter) or (filter and name == filter) %}

create-login-{{ name|yaml_encode }}:
  mdb_sqlserver.login_present:
    - name: {{ name|yaml_encode }}
    - password: {{ props['password']|yaml_encode }}
    - sid: {{ props['sid']|yaml_encode }}
    - server_roles:
{% for role in props.get('server_roles', []) %}
      - {{ role|yaml_encode }}
{% endfor %}
    - require:
      - test: sqlserver-logins-req
    - require_in:
      - test: sqlserver-logins-ready

{% endif %}
{% endfor %}

{% from "components/mysql/map.jinja" import mysql with context %}

mysql-users-req:
    test.nop

mysql-users-ready:
    test.nop

mysql-wait-online:
    cmd.run:
        - name: my-wait-online --wait 5m --defaults-file={{ mysql.defaults_file }}
        - onchanges:
            - service: mysql-service

{% set filter = salt['pillar.get']('target-user') %}
{% set idm_on = salt['pillar.get']('data:sox_audit', False) %}

{% set users, system_users, idm_system_users, idm_users = salt['mdb_mysql_users.get_users'](filter) %}

# create client users
{% for name, props in users.items() %}
{%     for host in salt['mdb_mysql.resolve_allowed_hosts'](props.get('hosts', '%')) %}
create-user-{{ name|yaml_encode }}-{{ host|yaml_encode }}:
  mdb_mysql.user_present:
    - name: {{ name|yaml_encode }}
    - password: {{ props['password']|yaml_encode }}
    - host: {{ host|yaml_encode }}
{% if props.get('plugin')  %}
    - auth_plugin: {{ props['plugin']|yaml_encode }}
{% endif %}
{% set limits = props.get('connection_limits') %}
{% if limits %}
    - connection_limits:
{% for limit, value in limits.items() %}
        {{ limit }}: {{ value }}
{% endfor %}
{% endif %}
    - connection_default_file: /home/mysql/.my.cnf
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%     endfor %}
{% endfor %}

# create system users
{% for name, props in system_users.items() %}
{%     for host in salt['mdb_mysql.resolve_allowed_hosts'](props.get('hosts', '%')) %}
create-user-{{ name|yaml_encode }}-{{ host|yaml_encode }}:
  mdb_mysql.user_present:
    - name: {{ name|yaml_encode }}
    - password: {{ props['password']|yaml_encode }}
    - host: {{ host|yaml_encode }}
{% if mysql.version.num >= 800 %}
    - auth_plugin: caching_sha2_password
{% else %}
    - auth_plugin: sha256_password
{% endif %}
{% set limits = props.get('connection_limits') %}
{% if limits %}
    - connection_limits:
{% for limit, value in limits.items() %}
        {{ limit }}: {{ value }}
{% endfor %}
{% endif %}
    - connection_default_file: /home/mysql/.my.cnf
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%     endfor %}
{% endfor %}

# grant proxy to admins
{% for name, props in system_users.items() %}
{%     if name == 'admin' %}
{%         for host in salt['mdb_mysql.resolve_allowed_hosts'](props.get('hosts', '%')) %}
grant-user-proxy-{{ name|yaml_encode }}-{{ host|yaml_encode }}:
  mdb_mysql_users.grant_proxy_grants:
    - name: {{ name|yaml_encode }}
    - host: {{ host|yaml_encode }}
    - connection_default_file: /home/mysql/.my.cnf
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%         endfor %}
{%     endif %}
{% endfor %}


{%   for name, props in idm_users.items() %}
manage-idm-user-lock-{{name|yaml_encode}}:
  mdb_mysql_users.manage_user_lock:
    - name: {{ name|yaml_encode }}
    - lock: {{ not idm_on }}
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%   endfor %}


{% if idm_on %}
{%   for name, props in idm_system_users.items() %}
create-idm-system-user-{{name|yaml_encode}}:
  mdb_mysql.idm_system_user_present:
    - name: {{ name|yaml_encode }}
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%   endfor %}


{%   for name, props in idm_users.items() %}
create-idm-user-{{name|yaml_encode}}:
  mdb_mysql.idm_user_present:
    - name: {{ name|yaml_encode }}
    - password: {{ props['password']|yaml_encode }}
{% if props.get('plugin') %}
    - auth_plugin: {{ props['plugin']|yaml_encode }}
{% endif %}
{% set limits = props.get('connection_limits') %}
{% if limits %}
    - connection_limits:
{% for limit, value in limits.items() %}
        {{ limit }}: {{ value }}
{% endfor %}
{% endif %}
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
      - mdb_mysql_users: manage-idm-user-lock-{{name|yaml_encode}}
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online

grant-proxy-idm-user-{{name|yaml_encode}}:
  mdb_mysql_users.user_grant_proxy_to:
    - name: {{ name|yaml_encode }}
    - proxy_user: {{ props['proxy_user']|yaml_encode }}
    - require:
      - test: mysql-users-req
      - test: mysql-service-ready
      - mdb_mysql: create-idm-user-{{name|yaml_encode}}
{% if idm_system_users and props['proxy_user'] %}
      - mdb_mysql: create-idm-system-user-{{props['proxy_user']|yaml_encode}}
{% endif %}
    - require_in:
      - test: mysql-users-ready
      - cmd: mysql-wait-online
{%   endfor %}

{% endif %}

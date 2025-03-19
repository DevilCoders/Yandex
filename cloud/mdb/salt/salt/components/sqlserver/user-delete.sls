{% set username = salt['pillar.get']('target-user') %}

{% for db in salt['pillar.get']('data:sqlserver:databases', {}) %}
user-absent-{{ username|yaml_encode }}-{{db|yaml_encode}}:
    mdb_sqlserver_user.absent:
        - name: {{ username|yaml_encode }}
        - database: {{ db|yaml_encode }} 
        - require_in:
            - mssql_login: login-absent-{{ username|yaml_encode }}
{% endfor %}

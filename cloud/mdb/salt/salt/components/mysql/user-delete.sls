{% set all_possible_host = ['%'] + salt['mdb_mysql.resolve_allowed_hosts']('__cluster__') %}
{% set name = salt['pillar.get']('target-user') %}

{% for host in all_possible_host %}
drop-user-{{ name|yaml_encode }}-{{ host|yaml_encode }}:
  mdb_mysql.user_absent:
    - name: {{ name|yaml_encode }}
    - host: {{ host|yaml_encode }}
    - connection_default_file: /home/mysql/.my.cnf
{% endfor %}

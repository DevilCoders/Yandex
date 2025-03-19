mysql-databases-req:
    test.nop

mysql-databases-ready:
    test.nop

{% set filter = salt['pillar.get']('target-database') %}

{% for dbname in salt['pillar.get']('data:mysql:databases') %}
{%    if (not filter) or (filter and dbname == filter) %}
create-db-{{ dbname|yaml_encode }}:
  mdb_mysql.database_present:
    - name: {{ dbname|yaml_encode }}
    - connection_default_file: /home/mysql/.my.cnf
    - lower_case_table_names: {{ salt['pillar.get']('data:mysql:config:lower_case_table_names', 0) }}
    - require:
      - test: mysql-databases-req
    - require_in:
      - test: mysql-databases-ready
{%     endif %}
{% endfor %}

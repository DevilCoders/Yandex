{% set name = salt['pillar.get']('target-database') %}

drop-db-{{ name|yaml_encode }}:
  mdb_mysql.database_absent:
    - name: {{ name|yaml_encode }}
    - connection_default_file: /home/mysql/.my.cnf
    - lower_case_table_names: {{ salt['pillar.get']('data:mysql:config:lower_case_table_names', 0) }}

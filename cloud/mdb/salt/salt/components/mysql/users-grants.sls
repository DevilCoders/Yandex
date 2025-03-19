mysql-ensure-grants:
  mdb_mysql.ensure_grants:
    - connection_default_file: /home/mysql/.my.cnf
{% if salt['pillar.get']('target-user') %}
    - target_user: {{ salt['pillar.get']('target-user')|yaml_encode }}
{% endif %}
{% if salt['pillar.get']('target-database') %}
    - target_database: {{ salt['pillar.get']('target-database')|yaml_encode }}
{% endif %}

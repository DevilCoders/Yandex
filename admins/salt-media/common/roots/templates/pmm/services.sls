{% if salt.pillar.get("pmm") %}

{% set pmm_server = salt.pillar.get("pmm:pmm_server") %}
{% set pmm_server_port = salt.pillar.get("pmm:pmm_server_port") %}
{% set pmm_db_type = salt.pillar.get("pmm:pmm_db_type") %}
{% if salt.pillar.get("pmm:pmm_mongo_cluster") %}
{% set pmm_mongo_cluster = salt.pillar.get("pmm:pmm_mongo_cluster") %}
{% set pmm_mongo_port = salt.pillar.get("pmm:pmm_mongo_port") %}
{% endif %}

pmm_install_script:
  cmd.script:
{% if pmm_db_type == "mysql" %}
    - source: salt://{{ slspath }}/pmm_mysql_install_script.sh
{% elif pmm_db_type == "mongo" %}
    - source: salt://{{ slspath }}/pmm_mongo_install_script.sh
{% endif %}
    - creates: /usr/local/percona/pmm-client/pmm.yml
    - env:
      - PATH: "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
      - PMM_SERVER: {{ pmm_server }}
      - PMM_SERVER_PORT: {{ pmm_server_port }}
{% if salt.pillar.get("pmm:pmm_mongo_cluster") %}
      - PMM_MONGO_CLUSTER: {{ pmm_mongo_cluster }}
      - PMM_MONGO_PORT: {{ pmm_mongo_port }}
{% endif %}

{% if salt.pillar.get("pmm:use_best_results_mysql_config") %}
/etc/mysql/conf.d/pmm.cnf:
  file.managed:
    - source: salt://{{ slspath }}/pmm.cnf
{% endif %}

{% endif %}

{% from "components/mysql/map.jinja" import mysql with context %}

{% set restore_raw = salt['pillar.get']('walg-restore', salt['pillar.get']('data:walg-restore', False)) %}
{% if restore_raw is string and restore_raw in ('MASTER', 'WALG', 'WALG-NO-BINLOGS') %}
    {% set mode = restore_raw %}
# true, WALG, some_other_string
{% elif restore_raw %}
    {% set mode = 'WALG' %}
{% else %}
    {% set mode = 'MASTER' %}
{% endif %}

mysql-init:
  mdb_mysql.replica_init2:
    - datadir: /var/lib/mysql
{% if salt.pillar.get('data:dbaas:flavor:cpu_guarantee') > 2 %}
    - backup_threads: {{ (salt.pillar.get('data:dbaas:flavor:cpu_guarantee') // 2)|int }}
{% endif %}
    - mode: {{mode}}
{% if mode == 'MASTER' %}
{% if salt['pillar.get']('data:mysql:replication_source') %}
    - server: {{ salt['pillar.get']('data:mysql:replication_source') }}
{% endif %}
{% if salt['pillar.get']('data:dbaas:flavor:cpu_guarantee') > 2 %}
    - backup_threads: {{ (salt['pillar.get']('data:dbaas:flavor:cpu_guarantee') // 2)|int }}
{% endif %}
    - use_memory: {{ (salt.pillar.get('data:dbaas:flavor:memory_guarantee', 1024**3) // 2)|int }}
{% else %}
    - walg_config: {{ mysql.walg_config }}
    - backup_name: 'LATEST'
    - max_live_binlog_wait: 3600
{% endif %}
    - connection_default_file: {{ mysql.defaults_file }}
    - require:
      - user: mysql-user
      - pkg: mysql
      - file: /home/mysql/.ssh
      - file: /var/lib/mysql
      - file: /var/lib/mysql/.tmp
      - file: /etc/mysql/my.cnf
      - file: {{ mysql.defaults_file }}
      - file: /usr/local/yandex/mysql_restore.py
      - mdb_mysql: mysql-find-and-set-master
      - test: walg-ready
    - require_in:
      - test: mysql-service-req


include:
    - .replica-setup-replication
    - .replica-set-online

extend:
    mysql-setup-replication:
        mdb_mysql.setup_replication:
            - require:
                - test: mysql-service-ready
                - test: mysql-initialized
                - file: {{ mysql.defaults_file }}
            - require_in:
                - test: mysql-ready
    replica-set-online:
        cmd.run:
            - require:
                - mdb_mysql: mysql-setup-replication

{% if salt['pillar.get']('restore-from:cid') %}
update-password-file:
  cmd.run:
    - name: cp -f /home/mysql/.my.cnf /home/mysql/.restore.my.cnf
    - require:
      - mdb_mysql: mysql-setup-replication
    - require_in:
      - test: mysql-ready
{% endif %}

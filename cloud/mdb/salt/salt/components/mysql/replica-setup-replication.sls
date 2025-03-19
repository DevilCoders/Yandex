{% from "components/mysql/map.jinja" import mysql with context %}

mysql-find-and-set-master:
  mdb_mysql.set_master:
    - user: admin
    - password: {{ salt['pillar.get']('data:mysql:users:admin:password') }}
    - timeout: {{ salt['pillar.get']('mysql-master-timeout', 600) }}
    - port: 3307
{% if salt['pillar.get']('restore-from:cid') %}
    - require:
      - cmd: mysql-restore
{% endif %}

mysql-setup-replication:
  mdb_mysql.setup_replication:
{% if salt['pillar.get']('data:mysql:replication_source') %}
    - server: {{ salt['pillar.get']('data:mysql:replication_source') }}
{% endif %}
    - user: repl
    - password: "{{ salt['pillar.get']('data:mysql:users:repl:password') }}"
    - connection_default_file: {{ mysql.defaults_file }}
    - master_port: 3307
{% if salt['pillar.get']('data:mysql:use_ssl', True) %}
    - master_ssl: true
    - master_ssl_ca: /etc/mysql/ssl/allCAs.pem
{% endif %}
    - require:
        - mdb_mysql: mysql-find-and-set-master
        - mysql-settings-req

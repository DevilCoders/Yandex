{% from "components/mysql/map.jinja" import mysql with context %}

mysql-audit-settings-req:
  test.nop

/etc/mysql/my_audit.cnf:
  file.managed:
    - user: mysql
    - group: mysql
    - mode: 755
    - mode: 644
    - makedirs: True
    - template: jinja
    - source: salt://components/mysql/conf/my_audit.cnf
    - require_in:
      - test: mysql-audit-settings-req

set-audit-variables:
  mdb_mysql.ensure_settings:
    - connection_default_file: {{ mysql.defaults_file }}
    - mycnf_file: /etc/mysql/my_audit.cnf
    - is_replica: {{ mysql.is_replica }}
    - require:
      - test: mysql-audit-settings-req

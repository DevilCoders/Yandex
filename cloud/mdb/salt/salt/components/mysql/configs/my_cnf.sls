{% from "components/mysql/map.jinja" import mysql with context %}

mysql-settings-req:
  test.nop

/etc/mysql/my.cnf:
  file.managed:
    - user: mysql
    - group: mysql
    - mode: 755
    - mode: 644
    - makedirs: True
    - template: jinja
    - names:
      - /etc/mysql/my.cnf:
        - source: salt://components/mysql/conf/my.cnf
      - /etc/mysql/my_audit.cnf:
        - source: salt://components/mysql/conf/my_audit.cnf
    - require_in:
      - test: mysql-settings-req

set-global-variables:
  mdb_mysql.ensure_settings:
    - connection_default_file: {{ mysql.defaults_file }}
    - mycnf_file: /etc/mysql/my.cnf
    - is_replica: {{ mysql.is_replica }}
    - require:
      - test: mysql-settings-req

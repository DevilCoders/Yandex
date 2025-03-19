{% from "components/mysql/map.jinja" import mysql with context %}

mysql-init:
  mdb_mysql.master_init:
    - datadir: /var/lib/mysql
    - initfile: /etc/mysql/init.sql
    - require:
      - user: mysql-user
      - pkg: mysql
      - file: /var/lib/mysql
      - file: /var/lib/mysql/.tmp
      - file: /etc/mysql/my.cnf
      - file: /etc/mysql/init.sql
      - file: /home/mysql/.my.cnf
      - file: /var/lib/mysql/.tmp
      - file: /var/run/mysqld
      - file: /root/.my.cnf
      - file: /usr/local/yandex/mysql_init_timezones.py
    - require_in:
      - test: mysql-service-req

include:
    - .master-set-writable

extend:
    set-master-writable:
      mdb_mysql.set_master_writable:
        - require:
          - file: {{ mysql.defaults_file }}
          - test: mysql-service-ready

{% if salt['pillar.get']('restore-from:cid') %}
update-admin-user:
  cmd.run:
    - name: mysql --defaults-file=/home/mysql/.restore.my.cnf < /etc/mysql/init.sql
    - require:
      - cmd: mysql-restore
      - test: mysql-service-ready
      - mdb_mysql: set-master-writable
      - file: /home/mysql/.restore.my.cnf
    - require_in:
      - test: mysql-initialized

update-password-file:
  cmd.run:
    - name: cp -f /home/mysql/.my.cnf /home/mysql/.restore.my.cnf
    - require:
      - cmd: update-admin-user
    - require_in:
      - test: mysql-initialized
{% endif %}

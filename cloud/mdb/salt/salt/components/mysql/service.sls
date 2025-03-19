{% from "components/mysql/map.jinja" import mysql with context %}

# all DEBs installed, configs configured
mysql-service-req:
    test.nop

# mysql started, and synced
mysql-service-ready:
    test.nop

mysql-service:
  service.running:
    - name: mysql
    - enable: True
    - reload: False
    - require:
      - test: mysql-service-req

mysql-wait-started:
  cmd.wait:
    - name: my-wait-started -w 60s --defaults-file={{ mysql.defaults_file }}
    - watch:
      - service: mysql-service
    - require_in:
      - test: mysql-service-ready

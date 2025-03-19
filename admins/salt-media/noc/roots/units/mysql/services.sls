telegraf:
  service.running:
    - enable: True
    - reload: True
    - require:
      - pkg: telegraf-package
    - watch:
      - file: /etc/telegraf/telegraf.d/mysql.conf

mysql:
  service.running:
    - enable: True
    - require:
      - pkg: mysql-server

mysync:
  service.running:
    - enable: True
    - require:
      - pkg: mdb-mysync

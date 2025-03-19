include:
  - units.mysql.files
  - units.mysql.packages
  - units.mysql.services

mysqld:
  file.directory:
    - name: /mysqld
    - mode: 754
    - makedirs: True
    - user: mysql
    - group: telegraf

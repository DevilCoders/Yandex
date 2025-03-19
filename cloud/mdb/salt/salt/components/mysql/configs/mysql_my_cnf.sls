/home/mysql/.my.cnf:
  file.managed:
    - user: mysql
    - group: mysql
    - mode: 400
    - template: jinja
    - source: salt://components/mysql/conf/admin.my.cnf

/home/mysql/.replica.my.cnf:
  file.managed:
   - user: mysql
   - group: mysql
   - mode: 0400
   - template: jinja
   - source: salt://components/mysql/conf/replica.my.cnf

/home/mysql/.restore.my.cnf:
  file.managed:
    - user: mysql
    - group: mysql
    - mode: 0400
    - template: jinja
    - source: salt://components/mysql/conf/restore.my.cnf
    - replace: False

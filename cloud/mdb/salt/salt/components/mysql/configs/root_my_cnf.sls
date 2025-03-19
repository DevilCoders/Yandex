/root/.my.cnf:
  file.managed:
    - user: root
    - group: root
    - mode: 400
    - template: jinja
    - source: salt://components/mysql/conf/admin.my.cnf

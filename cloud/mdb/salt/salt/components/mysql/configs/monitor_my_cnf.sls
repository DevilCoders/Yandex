/home/monitor/.my.cnf:
  file.managed:
    - user: monitor
    - group: monitor
    - mode: 400
    - template: jinja
    - source: salt://components/mysql/conf/monitor.my.cnf

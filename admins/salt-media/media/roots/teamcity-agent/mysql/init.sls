/opt/mysql:
  file.directory:
    - user: mysql
    - group: mysql
    - dir_mode: 755

/etc/mysql/my.cnf:
  file.managed:
    - source: salt://{{ slspath }}/my.cnf
    - user: root
    - group: root
    - mode: 644

/etc/default/mysql:
  file.managed:
    - source: salt://teamcity-agent/mysql/default
    - user: root
    - group: root
    - mode: 644

/etc/mysql/mysql_init_file:
  file.managed:
    - source: salt://teamcity-agent/mysql/mysql_init_file
    - user: root
    - group: root
    - mode: 644

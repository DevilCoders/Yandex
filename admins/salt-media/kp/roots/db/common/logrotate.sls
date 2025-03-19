/etc/logrotate.d/mysql-logs:
  file.managed:
    - source: salt://{{ slspath }}/files/mysql-logs.logrotate
    - user: root
    - group: root
    - mode: 644

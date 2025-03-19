/usr/sbin/mysql-staging-status.py:
  file.managed:
    - source: salt://{{slspath}}/usr/sbin/mysql-staging-status.py
    - mode: 755

/etc/monrun/salt_mysql_staging/MANIFEST.json:
  file.managed:
    - source: salt://{{slspath}}/etc/monrun/salt_mysql_staging/MANIFEST.json
    - makedirs: True

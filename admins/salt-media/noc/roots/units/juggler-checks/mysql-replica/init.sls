/etc/monrun/salt_mysql-replica/mysql-replica.pl:
  file.managed:
    - source: salt://{{ slspath }}/files/mysql-replica.pl
    - makedirs: True
    - mode: 755

/etc/monrun/salt_mysql-replica/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True


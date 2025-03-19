/etc/init.d/proftpd:
  file.managed:
    - source: salt://files/backup/etc/init.d/proftpd
    - mode: 755

/etc/mongodb.conf:
  file.managed:
    - source: salt://mongo/etc/mongodb.conf
/usr/local/bin/mongo-dump.sh:
  file.managed:
    - source: salt://mongo/usr/local/bin/mongo-dump.sh
/etc/monrun/conf.d/mongo-backup.conf:
  file.managed:
    - source: salt://mongo/etc/monrun/conf.d/mongo-backup.conf


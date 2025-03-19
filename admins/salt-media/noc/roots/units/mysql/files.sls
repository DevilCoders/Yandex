include:
  - units.mysql.mysync-config

/etc/dbaas.conf:
  file.managed:
    - source: salt://units/mysql/files/etc/dbaas.conf
    - template: jinja

/usr/sbin/mysql-backup.sh:
  file.managed:
    - source: salt://units/mysql/files/usr/sbin/mysql-backup.sh
    - mode: 755

/usr/sbin/mysql-backup-kvartal.sh:
  file.managed:
    - source: salt://units/mysql/files/usr/sbin/mysql-backup-kvartal.sh
    - mode: 755


/usr/sbin/mysync-maint.sh:
  file.managed:
    - source: salt://units/mysql/files/usr/sbin/mysync-maint.sh
    - mode: 755

/usr/sbin/mysql-status.sh:
  file.managed:
    - source: salt://units/mysql/files/usr/sbin/mysql-status.sh
    - mode: 755
    - template: jinja

/etc/cron.d/mysql-backup:
  file.managed:
    - source: salt://units/mysql/files/etc/cron.d/mysql-backup

/root/.my.cnf:
  file.managed:
    - source: salt://units/mysql/files/root/.my.cnf
    - template: jinja

/etc/mysql/my.cnf:
  file.managed:
    - source: salt://units/mysql/files/etc/mysql/my.cnf
    - template: jinja

/etc/mysql/mysql.conf.d/mysqld.cnf:
  file.managed:
    - source: salt://units/mysql/files/etc/mysql/mysql.conf.d/mysqld.cnf

/lib/systemd/system/mysync.service:
  file.managed:
    - source: salt://units/mysql/files//lib/systemd/system/mysync.service

/etc/apt/sources.list.d/mdb-bionic.list:
  file.managed:
    - source: salt://units/mysql/files/etc/apt/sources.list.d/mdb-bionic.list

/etc/telegraf/telegraf.d/mysql.conf:
  file.managed:
    - source: salt://units/mysql/files/etc/telegraf/telegraf.d/mysql.conf
    - template: jinja
    - mode: 640
    - group: telegraf

/etc/monrun/salt_mysql/MANIFEST.json:
  file.managed:
    - source: salt://units/mysql/files/etc/monrun/salt_mysql/MANIFEST.json
    - makedirs: True

/etc/logrotate.d/mysql-server:
  file.managed:
    - source: salt://units/mysql/files/etc/logrotate.d/mysql-server

/etc/logrotate.d/mysync:
  file.managed:
    - source: salt://units/mysql/files/etc/logrotate.d/mysync



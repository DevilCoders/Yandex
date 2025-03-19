include:
  - units.juggler-checks.common

/usr/local/bin/mysql-resetup-from-backup.sh:
  file.managed:
    - source: salt://files/nocdev-test-mysql/mysql-resetup-from-backup.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/mysql-resetup:
  file.managed:
    - source: salt://files/nocdev-test-mysql/etc/logrotate.d/mysql-resetup
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/var/log/mysql-resetup.log:
  file.managed:
    - mode: 666
    - user: robot-nocdev-mysql
    - group: root

/var/cache/mysql-resetup/:
  file.directory:
    - user: root
    - group: root
    - mode: 755

eatmydata:
  pkg.installed

sudo eatmydata /usr/local/bin/mysql-resetup-from-backup.sh /var/cache/mysql-resetup &>> /var/log/mysql-resetup.log:
  cron.present:
    - identifier: mysql-resetup-from-backup.sh
    - user: robot-nocdev-mysql
    - hour: 04
    - minute: 00

/etc/systemd/system/mysql.service.d/10-eatmydata.conf:
  file.managed:
    - contents: |
        [Service]
        Environment="LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libeatmydata.so"
    - user: root
    - group: root
    - makedirs: True

systemctl daemon-reload:
  cmd.run:
    - onchanges:
      - file: /etc/systemd/system/mysql.service.d/10-eatmydata.conf



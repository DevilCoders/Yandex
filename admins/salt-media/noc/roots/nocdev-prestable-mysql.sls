include:
  - units.juggler-checks.common

/usr/local/bin/mysql-resetup-cluster.sh:
  file.managed:
    - source: salt://files/nocdev-prestable-mysql/mysql-resetup-cluster.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/mysql-resetup.log:
  file.managed:
    - mode: 666
    - user: robot-nocdev-mysql
    - group: root

/root/.my.cnf.production:
  file.managed:
    - template: jinja
    - contents: |
        [client]
        user=admin
        host=localhost
        socket=/mysqld/mysqld.sock
        port=3306
        password={{ pillar['mysql_secrets_production']['mysync_admin_pas'] }}

        [mysql]
        default-character-set=utf8mb4


sudo /usr/local/bin/mysql-resetup-cluster.sh >> /var/log/mysql-resetup.log:
  cron.present:
    - user: robot-nocdev-mysql
    - hour: 04
    - minute: 00

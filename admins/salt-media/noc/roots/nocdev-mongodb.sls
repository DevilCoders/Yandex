include:
  - units.juggler-checks.common

/etc/apt/sources.list.d/mongodb-org-4.2.list:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/apt/sources.list.d/mongodb-org-4.2.list

/etc/mongod-keyfile.yml:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/mongod-keyfile.yml
    - user: mongodb
    - group: mongodb
    - mode: 400
    - template: jinja

/etc/mongod.conf:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/mongod.conf
    - template: jinja

/etc/cron.d/mongo-backup:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/cron.d/mongo-backup

/usr/sbin/mongo-backup.sh:
  file.managed:
    - source: salt://files/nocdev-mongo/usr/sbin/mongo-backup.sh
    - template: jinja
    - mode: 700

/etc/logrotate.d/mongodb:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/logrotate.d/mongodb
    - template: jinja

/etc/telegraf/telegraf.d/mongo.conf:
  file.managed:
    - source: salt://files/nocdev-mongo/etc/telegraf/telegraf.d/mongo.conf
    - template: jinja

telegraf:
  service.running:
    - reload: True
    - require:
      - pkg: telegraf
    - watch:
      - file: /etc/telegraf/telegraf.d/mongo.conf
  pkg:
    - installed
    - pkgs:
      - telegraf-noc-conf
      - telegraf

mongod:
  service:
    - running
    - require:
      - pkg: mongod
  pkg:
    - installed
    - pkgs:
      - mongodb-org-server
      - mongodb-org


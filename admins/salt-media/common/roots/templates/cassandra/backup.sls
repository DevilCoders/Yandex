{% from  slspath + "/map.jinja" import cassandra with context %}

{% if cassandra.backup.enabled %}
/etc/cassandra-backup/cassandra-backup.conf:
  file.managed:
    - source: salt://{{ slspath }}/etc/cassandra-backup/cassandra-backup.tmpl
    - template: jinja
    - makedirs: True
    - context:
        cassandra: {{ cassandra|json }}
cassandra-backup-package:
  pkg.installed:
  - pkgs:
      - yandex-media-common-cassandra-backup

/etc/cron.d/cassandra-backup:
  file.managed:
    - require:
      - pkg: cassandra-backup-package
    - contents:
      - SHELL=/bin/bash
      - 1 0 * * * root sleep $( expr $RANDOM \% 2700) ; /usr/bin/cassandra-backup-snapshot.sh >> /var/log/cassandra-backup.log 2>&1
{% else %}
cassandra.backup.diasabled:
  file.absent:
    - names:
      - /etc/cron.d/cassandra-backup
      - /etc/cassandra-backup/cassandra-backup.conf
{% endif %}

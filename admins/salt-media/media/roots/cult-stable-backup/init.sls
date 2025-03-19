include:
  - templates.proftpd.server
  - templates.rsyncd

/usr/bin/rcmnd-mongo-backup-status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/rcmnd-mongo-backup-status.sh
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

rcmnd-mongo-backup-status:
  monrun.present:
    - command: /usr/bin/rcmnd-mongo-backup-status.sh
    - type: rcmnd
    - execution_interval: 300

{% if 'backup-backup' not in salt['grains.get']('conductor:group') %}
install-postgresql-repo:
  pkgrepo.managed:
    - humanname: PostgreSQL Official Repository
    - keyid: B97B0AFCAA1A47F044F244A07FCC7D46ACCC4CF8
    - keyserver: keys.openpgp.org
    - file: /etc/apt/sources.list.d/pgdg.list
    - name: deb http://apt.postgresql.org/pub/repos/apt/ precise-pgdg main 9.4

pg_client:
  pkg.installed:
    - name: postgresql-client-common
    - refresh: true

barman:
  pkg.installed:
    - name: barman
    - refresh: true

/usr/local/barman-recover/barman-recover.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/barman-recover/barman-recover.py
    - user: barman
    - group: barman
    - mode: 0755
    - makedirs: True

barman_symlink:
  file.symlink:
    - name: /usr/bin/barman-recover.py
    - target: /usr/local/barman-recover/barman-recover.py
    - user: barman
    - group: barman
    - mode: 744

/usr/local/barman-recover/utilities.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/barman-recover/utilities.py
    - user: barman
    - group: barman
    - mode: 0755
    - makedirs: True

/etc/barman-recover/weather-load-pg.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/barman-recover/weather-load-pg.conf
    - user: barman
    - group: barman
    - mode: 0755
    - makedirs: True

/etc/cron.d/barman-recover:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/barman-recover
    - user: root
    - group: root
    - mode: 0755

/etc/cron.d/barman:
  file.managed:
    - source: salt://{{ slspath }}/files/barman/barman.cron
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - replace: True

/usr/bin/make-backup-of-backup.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/make-backup-of-backup.sh
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

lockf -t 0 /tmp/make-backup-of-backup.lock /usr/bin/make-backup-of-backup.sh:
  cron.present:
    - user: root
    - hour: '15'
    - minute: '05'
    - identifier: makebackupofbackup

/usr/bin/check-backup-of-backup.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/check-backup-of-backup.sh
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

check-backup-of-backup:
  monrun.present:
    - command: /usr/bin/check-backup-of-backup.sh
    - type: cult
    - execution_interval: 300
    - execution_timeout: 30
{% endif %}

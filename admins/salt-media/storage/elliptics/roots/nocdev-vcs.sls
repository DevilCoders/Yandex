/etc/cron.d/gitlab-backup:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/etc/cron.d/gitlab-backup

/etc/rsyncd.conf:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/etc/rsyncd.conf
    - user: root
    - group: root
    - mode: 644

/etc/rsyncd.passwd:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/etc/rsyncd.passwd
    - user: root
    - group: root
    - mode: 600
    - template: jinja

/data/gitlab/backup.exclude:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/backup.exclude
    - user: root
    - group: root
    - mode: 600

rsync:
  service:
    - running
    - restart: True
    - require:
      - pkg: rsync
  pkg:
    - installed
    - pkgs:
      - rsync

zstd:
  pkg:
    - installed
    - pkgs:
        - zstd

/usr/sbin/gitlab-backup.sh:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/usr/sbin/gitlab-backup.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/usr/sbin/gitlab-backup-fetch.sh:
  file.managed:
    - source: salt://files/nocdev-gitlab-backup/usr/sbin/gitlab-backup-fetch.sh
    - user: root
    - group: root
    - mode: 750
    - makedirs: True
    - template: jinja


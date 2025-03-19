include:
  - .rsyslog-config

/etc/cron.d/gitlab-backup:
  file.managed:
    - source: salt://units/gitlab/files/etc/cron.d/gitlab-backup

/etc/rsyncd.conf:
  file.managed:
    - source: salt://units/gitlab/files/etc/rsyncd.conf
    - user: root
    - group: root
    - mode: 644

/etc/rsyncd.passwd:
  file.managed:
    - source: salt://units/gitlab/files/etc/rsyncd.passwd
    - user: root
    - group: root
    - mode: 600
    - template: jinja

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
    - source: salt://units/gitlab/files/usr/sbin/gitlab-backup.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/usr/sbin/gitlab-backup-fetch.sh:
  file.managed:
    - source: salt://units/gitlab/files/usr/sbin/gitlab-backup-fetch.sh
    - user: root
    - group: root
    - mode: 750
    - makedirs: True
    - template: jinja

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://units/nginx_conf/files/logrotate-nginx
    - makedirs: True

/etc/cron.d/docker-clean:
  file.managed:
    - source: salt://units/gitlab/files/etc/cron.d/docker-clean
    - makedirs: True

/data/gitlab/yandex-wrapper:
  file.managed:
    - source: salt://units/gitlab/files/data/gitlab/yandex-wrapper
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

ch-backup-main-req:
    test.nop

ch-backup-main-ready:
    test.nop

ch-backup-packages:
    pkg.installed:
        - pkgs:
            - ch-backup: 2.367.75859421
            - python3.6
            - python3.6-minimal
            - libpython3.6-stdlib
            - libpython3.6-minimal
            - zk-flock
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
            - s3cmd
            - libmpdec2
        - require:
            - test: ch-backup-main-req
        - require_in:
            - test: ch-backup-main-ready

/etc/yandex/ch-backup/s3cmd.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/s3cmd.conf
        - template: jinja
        - user: root
        - mode: 640
        - makedirs: True
        - require:
            - pkg: ch-backup-packages
        - require_in:
            - test: ch-backup-main-ready

/etc/logrotate.d/ch-backup:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True
        - require:
            - pkg: ch-backup-packages
        - require_in:
            - test: ch-backup-main-ready

/var/log/ch-backup:
    file.directory:
        - dir_mode: 775
        - makedirs: True
        - require:
            - pkg: ch-backup-packages
        - require_in:
            - test: ch-backup-main-ready

/etc/cron.yandex/ch-backup.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/ch-backup.sh
        - mode: 755
        - makedirs: True
        - require:
            - pkg: ch-backup-packages
        - require_in:
            - test: ch-backup-main-ready

dbm-cron-deps:
    pkg.installed:
        - pkgs:
            - python3-requests

/etc/cron.d/dbm_expire_secrets:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_expire_secrets.cron
        - mode: 644
        - user: root

/etc/cron.yandex/dbm_expire_secrets.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_expire_secrets.py
        - mode: 755
        - user: root

/etc/cron.d/dbm_expire_clusters:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_expire_clusters.cron
        - mode: 644
        - user: root

/etc/cron.yandex/dbm_expire_clusters.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_expire_clusters.py
        - mode: 755
        - user: root

/etc/cron.d/dbm_expire_backups:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/dbm_expire_backups.cron
        - mode: 644
        - user: root

/etc/cron.yandex/dbm_expire_backups.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_expire_backups.py
        - mode: 755
        - user: root

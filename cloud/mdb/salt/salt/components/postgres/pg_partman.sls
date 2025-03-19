/etc/cron.yandex/pg_partman_maintenance.py:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pg_partman_maintenance.py
        - mode: 755
        - user: postgres
        - require:
            - file: /etc/cron.yandex

/etc/cron.d/pg_partman:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/cron.d/pg_partman
        - mode: 644
        - require:
            - file: /etc/cron.yandex/pg_partman_maintenance.py

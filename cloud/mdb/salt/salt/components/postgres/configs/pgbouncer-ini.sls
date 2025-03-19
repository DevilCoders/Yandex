/etc/pgbouncer/pgbouncer.ini:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pgbouncer.ini
        - mode: 644
        - user: root
        - group: root
        - makedirs: True

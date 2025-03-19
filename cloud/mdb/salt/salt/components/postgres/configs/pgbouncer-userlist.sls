/etc/pgbouncer/userlist.txt:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/userlist.txt
        - mode: 755
        - user: root
        - group: root
        - makedirs: True

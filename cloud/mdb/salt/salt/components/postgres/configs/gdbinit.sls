/root/.gdbinit:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/gdbinit.conf
        - mode: 644
        - user: root
        - group: root

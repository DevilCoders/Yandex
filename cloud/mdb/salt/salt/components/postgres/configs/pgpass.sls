{% from "components/postgres/pg.jinja" import pg with context %}

/root/.pgpass:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pgpass
        - user: root
        - group: root
        - mode: 600

/home/monitor/.pgpass:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pgpass
        - user: monitor
        - group: monitor
        - mode: 600

{{ pg.prefix }}/.pgpass:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/common-pgpass
        - user: postgres
        - group: postgres
        - mode: 600

{{ pg.prefix }}/.server-pgpass:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/server-pgpass
        - user: postgres
        - group: postgres
        - mode: 600

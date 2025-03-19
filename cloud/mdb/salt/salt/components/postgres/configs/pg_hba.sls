{% from "components/postgres/pg.jinja" import pg with context %}
{{ pg.data }}/conf.d/pg_hba.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pg_hba.conf
        - mode: 644
        - user: postgres
        - group: postgres

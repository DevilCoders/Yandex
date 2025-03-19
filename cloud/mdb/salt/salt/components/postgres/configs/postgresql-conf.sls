{% from "components/postgres/pg.jinja" import pg with context %}
{{ pg.data }}/conf.d/postgresql.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/postgresql.conf
        - mode: 644
        - user: postgres
        - group: postgres

{% if pg.version.major_num >= 1200 %}
{{ pg.data }}/postgresql.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/postgresql-include-only.conf
        - mode: 644
        - user: postgres
        - group: postgres
        - require:
            - file: {{ pg.data }}/conf.d/postgresql.conf
{% endif %}

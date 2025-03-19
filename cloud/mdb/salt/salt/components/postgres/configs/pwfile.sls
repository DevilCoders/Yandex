{% from "components/postgres/pg.jinja" import pg with context %}

{{ pg.prefix }}/.pwfile:
    file.managed:
        - contents: {{ salt['pillar.get']('data:config:pgusers:postgres:password', '') }}
        - user: postgres
        - group: postgres
        - mode: 0600

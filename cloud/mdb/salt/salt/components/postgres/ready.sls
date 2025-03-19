{% from "components/postgres/pg.jinja" import pg with context %}

postgresql-ready:
  test.nop:
    - require:
      - cmd: postgresql-service
      - file: /root/.pgpass
      - file: /home/monitor/.pgpass
      - file: {{ pg.prefix }}/.pgpass

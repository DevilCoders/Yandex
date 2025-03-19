{% from "components/postgres/pg.jinja" import pg with context %}

postgresql-stop:
    cmd.run:
        - name: /usr/local/yandex/pg_stop.sh
        - runas: postgres
        - group: postgres
        - onlyif:
            - '{{ pg.bin_path }}/pg_isready -q'
        - require_in:
            - service: postgresql-service
        - require:
            - file: {{ pg.data }}/conf.d/postgresql.conf

pooler-stop:
    service.dead:
        - name: {{ pg.connection_pooler }}
        - require_in:
            - cmd: postgresql-stop

pooler-start:
    service.running:
        - name: {{ pg.connection_pooler }}
        - require:
            - cmd: postgresql-service

make-checkpoint-after-restart:
    postgresql_cmd.psql_exec:
        - name: 'CHECKPOINT'
        - require:
            - cmd: postgresql-service

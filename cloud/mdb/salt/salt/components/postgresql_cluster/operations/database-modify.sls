{% from "components/postgres/pg.jinja" import pg with context %}
include:
{% if pg.connection_pooler == 'odyssey' %}
    - components.postgres.odyssey
{% endif %}
    - components.postgres.configs.pg_hba
    - components.postgres.service
    - components.pg-dbs.unmanaged.sync-databases

extend:
    postgresql-service:
        service.running:
            - watch:
                - file: {{ pg.data }}/conf.d/pg_hba.conf

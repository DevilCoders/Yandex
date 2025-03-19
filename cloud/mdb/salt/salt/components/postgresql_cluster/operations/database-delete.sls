{% from "components/postgres/pg.jinja" import pg with context %}
include:
{% if pg.connection_pooler == 'odyssey' %}
    - components.postgres.odyssey
{% else %}
    - components.postgres.pgbouncer
    - components.postgres.configs.pgbouncer-ini
{%     if salt['pillar.get']('data:pgbouncer:count', 1) > 1 %}
    - components.postgres.habouncer_systemd
{%     endif %}
{% endif %}
    - components.postgres.configs.pg_hba
    - components.postgres.service
    - components.pg-dbs.unmanaged.sync-databases
{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
    - components.pg-dbs.unmanaged.mdb-metrics
    - components.mdb-metrics.service
{% endif %}

extend:
    postgresql-service:
        service.running:
            - watch:
                - file: {{ pg.data }}/conf.d/pg_hba.conf

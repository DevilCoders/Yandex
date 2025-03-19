{% from "components/postgres/pg.jinja" import pg with context %}
include:
    - components.postgres.pkgs_ubuntu
{% if pg.connection_pooler == 'odyssey' %}
    - components.postgres.odyssey
{% else %}
    - components.postgres.pgbouncer
    - components.postgres.configs.pgbouncer-ini
{%     if salt['pillar.get']('data:pgbouncer:count', 1) > 1 %}
    - components.postgres.habouncer_systemd
{%     endif %}
{% endif %}
{% if not salt['pillar.get']('include-metadata', False) %}
    - components.postgres.pgsync
{%     if salt['pillar.get']('data:use_walg', True) %}
    - components.postgres.walg-config
{%     endif %}
{% endif %}
    - components.postgres.configs.postgresql-conf
{% if pg.is_replica %}
    - components.postgres.configs.recovery-conf
{%     if not salt['pillar.get']('data:pgsync:replication_source') %}
    - components.postgres.set-master-pillar
{%     endif %}
{%     if salt['pillar.get']('data:use_replication_slots', True) %}
    - components.postgres.create-replication-slot
{%     endif %}
{% endif %}
    - components.postgres.service
{% if salt['pillar.get']('service-restart') %}
    - components.postgres.restart
{% endif %}
{% if salt['pillar.get']('data:perf_diag:enable', False) %}
    - components.pg-dbs.unmanaged.perf-diag-service-enabled
{% elif salt['pillar.get']('data:ship_logs', False) and not salt['pillar.get']('data:perf_diag:enable', False)  %}
    - components.pg-dbs.unmanaged.perf-diag-service-disabled
{% endif %}
{% if salt['pillar.get']('users-modify') %}
    - components.postgres.users
{% endif %}

extend:
    postgresql-service:
        service.running:
            - watch:
                - file: {{ pg.data }}/conf.d/postgresql.conf
{% if pg.is_replica %}
                - postgresql_cmd: recovery.conf
{%     if not salt['pillar.get']('data:pgsync:replication_source') %}
    recovery.conf:
        postgresql_cmd.populate_recovery_conf:
            - require:
                - postgresql_cmd: set-master-pillar
{%     endif %}
{%     if salt['pillar.get']('data:use_replication_slots', True) %}
    create_replication_slot:
        postgresql_cmd.create_replication_slot:
{%         if not salt['pillar.get']('data:pgsync:replication_source') %}
            - require:
                - postgresql_cmd: set-master-pillar
{%         endif %}
            - require_in:
                - service: postgresql-service
{%     endif %}
{% endif %}

{% from "components/postgres/pg.jinja" import pg with context %}
include:
    - components.logrotate
    - components.monrun2.pg-common
    - .pkgs
    - .pkgs_additional
    - .groups
    - .directories
    - .configs
    - .sysctl
    - .repack
    - .plugins
{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
    - .mdb-metrics
{% endif %}
{% if pg.connection_pooler == 'pgbouncer' %}
    - .pgbouncer-install
{% else %}
    - components.odyssey.odyssey-pg
    - .odyssey
{% endif %}
    - .pkgs_ubuntu
    - .purge_old_versions
    - .ready
{% if pg.is_replica %}
    - .replica
{% else %}
    - .master
    - .extensions
{% endif %}
    - .users
{% if salt['pillar.get']('service-restart', False) %}
    - .restart
{% endif %}
    - components.yasmagent
    - .yasmagent
{% if salt['pillar.get']('data:diagnostic_tools', True) %}
    - .diagnostics
{% endif %}
{% if salt['pillar.get']('data:separate_array_for_xlogs', False) %}
    - .xlogs
{% endif %}
{% if salt['pillar.get']('data:use_pgsync', True) %}
    - .pgsync-install
{% endif %}
{% if salt['pillar.get']('data:separate_array_for_sata', False) %}
    - .sata
{% endif %}
    - .sqls
{% if salt['pillar.get']('data:use_pg_partman', False) %}
    - .pg_partman
{% endif %}
{% if salt['pillar.get']('data:use_postgis', False) %}
    - .postgis
{% endif %}
{% if salt['pillar.get']('data:pg_ssl', True) %}
    - .ssl
{% endif %}
{% if pg.connection_pooler == 'pgbouncer' and salt['pillar.get']('data:pgbouncer:count', 1) > 1 %}
    - .habouncer_systemd-install
{% endif %}
{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
    - components.mdb-metrics
{% else %}
    - components.mdb-metrics.disable
{% endif %}
{% if salt['pillar.get']('data:use_walg', True) %}
    - .walg
{% else %}
    - .walg_absent
{% endif %}
{% if salt['pillar.get']('data:ship_logs', False) %}
    - components.pushclient2
    - .pushclient
    - .remove_logs
{% endif %}
{% if salt['pillar.get']('data:dbaas:cluster') %}
    - .resize
{% if salt['pillar.get']('data:dbaas:vtype') in ['compute', 'porto'] %}
    - components.firewall
    - components.firewall.external_access_config
    - .firewall
{% endif %}
{% endif %}
{% if salt['pillar.get']('data:dbaas:shard_hosts') and salt['pillar.get']('data:use_replication_slots', True) %}
    - .sync-replication-slots
{% endif %}

extend:
    pg_sync_users:
        mdb_postgresql.sync_users:
            - require:
                - cmd: postgresql-service
                - pkg: python-psycopg2
                - pkg: python3-psycopg2


{% if salt['pillar.get']('data:dbaas:shard_hosts') and salt['pillar.get']('data:use_replication_slots', True) %}
    sync-replication-slots-req:
        test.nop:
            - require:
                - cmd: postgresql-service
{% endif %}

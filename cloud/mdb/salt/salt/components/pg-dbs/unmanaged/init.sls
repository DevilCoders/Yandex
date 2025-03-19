{% from "components/postgres/pg.jinja" import pg with context %}
{% set migrations_path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/unmanaged') %}

include:
    - .mdb-metrics
    - .sync-databases
    - .sync-extensions
{% if salt['pillar.get']('data:perf_diag:enable', False) %}
    - .perf-diag-enabled
{% elif salt['pillar.get']('data:ship_logs', False) and not salt['pillar.get']('data:perf_diag:enable', False)  %}
    - .perf-diag-disabled
{% endif %}

extend:
    pg_sync_databases:
        mdb_postgresql.sync_databases:
            - require:
                - mdb_postgresql: pg_sync_users
            - require_in:
                - sync-extensions

{{ migrations_path }}:
    file.recurse:
        - source: salt://components/pg-code/unmanaged
        - user: postgres
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - require:
            - service: postgresql-service

{% if pg.get('is_master') %}
unmanaged-schemas-apply:
    postgresql_schema.applied:
        - name: unmanaged
        - target: 3
        - conn: >
            host=localhost\ dbname=postgres\ user=postgres\ connect_timeout=1\ options=\'-c\ default_transaction_read_only=off\'
{% if salt['pillar.get']('data:config:pg_hint_plan_enable_hint', False) %}
        - session: >
            SET\ pg_hint_plan.enable_hint\ =\ off
{% endif %}
        - callbacks:
            afterAll:
                - /usr/local/yandex/sqls/show_activity_stats.sql
                - /usr/local/yandex/sqls/unapplied_settings.sql
        - require:
            - pkg: pgmigrate-pkg
            - service: postgresql-service
            - file: {{ migrations_path }}
            - file: /usr/local/yandex/sqls/show_activity_stats.sql
            - file: /usr/local/yandex/sqls/unapplied_settings.sql
        - require_in:
            - mdb_postgresql: pg_sync_users
{% endif %}

/etc/cron.d/disk_usage_watcher:
    file.managed:
        - source: salt://{{slspath}}/conf/disk_usage_watcher.cron.d
        - mode: 644
        - template: jinja

/usr/local/yandex/disk_usage_watcher.py:
    file.managed:
        - source: salt://{{slspath}}/conf/disk_usage_watcher.py
        - mode: 755

psycopg2-packages:
    pkg.installed:
        - pkgs:
            - python3-psycopg2

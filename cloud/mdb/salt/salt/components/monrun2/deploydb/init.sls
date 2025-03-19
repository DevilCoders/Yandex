{% from "components/postgres/pg.jinja" import pg with context %}
monrun-deploydb-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-deploydb-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

{% if pg.is_master %}
minions-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant select on deploy.minions to monitor'
        - maintenance_db: deploydb
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'deploy.minions', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: deploydb-schemas-apply
            - postgresql_cmd: deploy-schema-monitor-permission
        - require_in:
            - cmd: monrun-jobs-update

groups-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant select on deploy.groups to monitor'
        - maintenance_db: deploydb
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'deploy.groups', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: deploydb-schemas-apply
            - postgresql_cmd: deploy-schema-monitor-permission
        - require_in:
            - cmd: monrun-jobs-update

deploy-schema-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant usage on schema deploy to monitor'
        - maintenance_db: deploydb
        - unless:
            - -tA -c "select has_schema_privilege('monitor', 'deploy', 'usage');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: deploydb-schemas-apply
{% endif %}
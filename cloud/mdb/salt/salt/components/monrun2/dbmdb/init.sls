{% from "components/postgres/pg.jinja" import pg with context %}
monrun-dbmdb-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-dbmdb-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

{% if pg.is_master %}
transfers-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant select on mdb.transfers to monitor'
        - maintenance_db: dbm
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'mdb.transfers', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: dbm-schemas-apply
            - postgresql_cmd: mdb-schema-monitor-permission
        - require_in:
            - cmd: monrun-jobs-update

dom0hosts-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant select on mdb.dom0_hosts to monitor'
        - maintenance_db: dbm
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'mdb.dom0_hosts', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: dbm-schemas-apply
            - postgresql_cmd: mdb-schema-monitor-permission
        - require_in:
            - cmd: monrun-jobs-update

mdb-schema-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant usage on schema mdb to monitor'
        - maintenance_db: dbm
        - unless:
            - -tA -c "select has_schema_privilege('monitor', 'mdb', 'usage');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - postgresql_schema: dbm-schemas-apply
{% endif %}

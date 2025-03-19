{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}
{% from "components/postgres/pg.jinja" import pg with context %}
monrun-pg-common-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-pg-common-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-pg-common-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0640'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

{% if pg.is_master %}
repl-mon-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: 'grant select on repl_mon to monitor'
        - runas: postgres
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'public.repl_mon', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
{% endif %}

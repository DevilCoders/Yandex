{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}
{% set host = salt['pillar.get']('data:pgmeta:server', 'pgmeta.mail.yandex.net') %}
{% set port = salt['pillar.get']('data:pgmeta:read_port', '5432') %}
{% set pillar_db_users_map = salt['pillar.get']('data:config:db_users_map', {}) %}

{%
    set db_users_map = {
        'rpopdb': {'user': 'rpop',
                   'code': ['pgproxy/rpopdb.sql']},
        's3db': {
            'user': 's3api',
            'code': []
            }
    }
%}

{% do db_users_map.update(pillar_db_users_map) %}

{% for dbname in salt['pillar.get']('data:config:databases', []) %}
{# Remember that the order is important! #}
{%
    set sqls = [
        'pgproxy/pgproxy.sql',
        'pgproxy/get_cluster_config.sql',
        'pgproxy/get_cluster_partitions.sql',
        'pgproxy/get_cluster_version.sql',
        'pgproxy/inc_cluster_version.sql',
        'pgproxy/is_master.sql',
        'pgproxy/dynamic_query.sql',
        'pgproxy/get_partitions.sql',
        'pgproxy/select_part.sql'
    ] + db_users_map[dbname]['code']
%}


create_db_{{ dbname }}:
    postgres_database.present:
        - name: {{ dbname }}
        - user: postgres
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users

update_remote_tables_{{ dbname }}_copy:
    cmd.wait:
{% if dbname != 's3db' %}
        - name: cp -a {{ path }}pgproxy/update_remote_tables.sql {{ path }}pgproxy/update_remote_tables_{{ dbname }}.sql
        - watch:
            - file: {{ path }}pgproxy/update_remote_tables.sql
{% else %}
        - name: cp -a {{ path }}pgproxy/{{ dbname }}/update_remote_tables.sql {{ path }}pgproxy/update_remote_tables_{{ dbname }}.sql
        - watch:
            - file: {{ path }}pgproxy/{{ dbname }}
{%endif %}

update_remote_tables_{{ dbname }}_replace_grants:
    file.blockreplace:
        - name: /usr/local/yandex/pgproxy/update_remote_tables_{{ dbname }}.sql
        - marker_start: '--start of the section for grants'
        - marker_end: '--end of the section for grants'
        - content: |
            grant usage on schema plproxy to {{ db_users_map[dbname]['user'] }};
            grant all on all tables in schema plproxy to {{ db_users_map[dbname]['user'] }};
            grant execute on all functions in schema plproxy to {{ db_users_map[dbname]['user'] }};
{% if dbname == 's3db' %}
            grant select on all tables in schema plproxy to s3cleanup;
            grant usage on schema plproxy to s3api_ro;
            grant select on all tables in schema plproxy to s3api_ro;
            grant execute on all functions in schema plproxy to s3api_ro;
            grant usage on schema plproxy to s3api_list;
            grant select on all tables in schema plproxy to s3api_list;
            grant execute on all functions in schema plproxy to s3api_list;
{% endif %}
            grant usage on schema plproxy to monitor;
            grant select on all tables in schema plproxy to monitor;
        - require_in:
            - cmd: apply_code_{{ dbname }}_update_remote_tables
        - require:
            - cmd: update_remote_tables_{{ dbname }}_copy
        - watch:
            - cmd: update_remote_tables_{{ dbname }}_copy

update_remote_tables_{{ dbname }}_replace_fdw:
    file.blockreplace:
        - name: /usr/local/yandex/pgproxy/update_remote_tables_{{ dbname }}.sql
        - marker_start: '--start of the section for FDW'
        - marker_end: '--end of the section for FDW'
        - content: |
            create server remote foreign data wrapper postgres_fdw options (host '{{ host }}', dbname '{{ dbname }}', port '{{ port }}');
            create user mapping for postgres server remote options (user 'pgproxy', password '{{ salt['pillar.get']('data:config:pgusers:pgproxy:password', '') }}');
        - require_in:
            - cmd: apply_code_{{ dbname }}_update_remote_tables
        - require:
            - cmd: update_remote_tables_{{ dbname }}_copy
        - watch:
            - cmd: update_remote_tables_{{ dbname }}_copy

{% for sql in sqls %}
{% if not (dbname == 's3db' and sql == 'pgproxy/get_cluster_partitions.sql') %}
apply_code_{{ dbname }}_{{ sql }}:
    postgresql_cmd.psql_file:
        - name: {{ path + sql }}
        - maintenance_db: {{ dbname }}
        - onchanges:
            - file: {{ path + sql }}
            - postgres_database: create_db_{{ dbname }}
{% endif %}
{% endfor %}

{% if dbname == 's3db' %}
{%
    set s3_sqls = [
        'pgproxy/s3db/plproxy.sql',
        'pgproxy/s3db/v1/code.sql',
        'pgproxy/s3db/v1/impl.sql'
    ]
%}
apply_code_{{ dbname }}:
    cmd.wait:
        - name: cat{% for sql in s3_sqls %} {{ path + sql }}{% endfor %} | psql {{ dbname }} --single-transaction --set ON_ERROR_STOP=1
        - runas: postgres
        - group: postgres
        - watch:
            - file: {{ path }}pgproxy/s3db
            - postgres_database: create_db_{{ dbname }}
{% endif %}

apply_code_{{ dbname }}_update_remote_tables:
    postgresql_cmd.psql_file:
        - name: {{ path }}pgproxy/update_remote_tables_{{ dbname }}.sql
        - maintenance_db: {{ dbname }}
        - onchanges:
            - file: update_remote_tables_{{ dbname }}_replace_grants
            - file: update_remote_tables_{{ dbname }}_replace_fdw
            - postgres_database: create_db_{{ dbname }}
            - mdb_postgresql: pg_sync_users

first_init_{{ dbname }}_update_remote_tables:
    postgresql_cmd.psql_file:
        - name: {{ path }}pgproxy/call_update_remote_tables.sql
        - maintenance_db: {{ dbname }}
        - onchanges:
            - postgres_database: create_db_{{ dbname }}
            - mdb_postgresql: pg_sync_users
{% endfor %}

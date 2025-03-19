{% set dbname = 's3meta' %}
{% set target = salt['pillar.get']('data:s3meta:target', 'latest') %}
{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}
{% set code_versions = salt['pillar.get']('data:s3meta:versions', [1, ]) %}

{# Remember that the order is important! #}
{%
    set sqls = [
        'common/dynamic_query.sql',
        'common/is_master.sql',
        'common/check_partitions.sql',
        'common/pgcheck_poll.sql'
    ]
%}

include:
    - components.monrun2.s3
    - components.pg-dbs.s3
    - .mdb-metrics

{% for sql in sqls %}
{{ dbname + path + sql }}:
    file.managed:
        - source: salt://components/pg-code/{{ sql }}
        - name: {{ path + sql }}
        - user: postgres
        - template: jinja
        - mode: 744
        - makedirs: True
        - require:
            - cmd: postgresql-service
{% endfor %}

{{ path + dbname }}:
    file.recurse:
        - source: salt://components/pg-code/{{ dbname }}
        - user: postgres
        - template: jinja
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - require:
            - service: postgresql-service
        - defaults:
            default_dbname: {{ dbname }}

{% if salt['grains.get']('pg') and 'role' in salt['grains.get']('pg').keys() and salt['grains.get']('pg')['role'] == 'master' %}

create_db_{{ dbname }}:
    postgres_database.present:
        - name: {{ dbname }}
        - user: postgres
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users

{{ dbname + '-schemas-apply' }}:
    postgresql_schema.applied:
        - name: {{ dbname }}
        - target: {{ target }}
        - termination_interval: 0.1
        - conn: 'host=localhost\ dbname={{ dbname }}\ user=postgres\ connect_timeout=1'
        - callbacks:
            afterAll:
{% for sql in sqls %}
                - {{ path + sql }}
{% endfor %}
{% for version in code_versions %}
                - {{ path + dbname }}/v{{ version }}/code
                - {{ path + dbname }}/v{{ version }}/impl
                - {{ path + dbname }}/v{{ version }}/grants
{% endfor %}
        - require:
            - postgres_database: create_db_{{ dbname }}
            - pkg: pgmigrate-pkg
            - file: {{ path + dbname }}
{% for sql in sqls %}
            - file: {{ path + sql }}
{% endfor %}
{% endif %}

{%
    set pgmeta_hosts = {
        'prod': 'pgmeta.mail.yandex.net',
        'load': 'pgmeta-load01h.cmail.yandex.net',
        'dev': 'pgmeta-test01h.mail.yandex.net',
    }
%}


{% set pgmeta_host = salt['pillar.get']('data:config:pgmeta:server', pgmeta_hosts[salt['pillar.get']('yandex:environment', 'dev')]) %}
{% set pgmeta_port = salt['pillar.get']('data:config:pgmeta:read_port', '5432') %}
{% set pgmeta_dbname = salt['pillar.get']('data:config:pgmeta:dbname', 's3db') %}
{% set srvname = 'pgmeta' %}

{% if salt['grains.get']('pg') and 'role' in salt['grains.get']('pg').keys() and salt['grains.get']('pg')['role'] == 'master' %}
{{ dbname }}-create-extension-postgres_fdw:
    postgresql_cmd.psql_exec:
        - name: 'create extension postgres_fdw cascade'
        - maintenance_db: {{ dbname }}
        - runas: postgres
        - require:
            - postgres_database: create_db_{{ dbname }}
        - unless:
            - -c "select extname from pg_catalog.pg_extension;" | grep postgres_fdw

{{ dbname }}-create-{{ srvname }}-foreign-server:
    postgresql_cmd.psql_exec:
        - name: "DROP SERVER IF EXISTS {{ srvname }} CASCADE; CREATE SERVER {{ srvname }} FOREIGN DATA WRAPPER postgres_fdw OPTIONS (host '{{ pgmeta_host }}', port '{{ pgmeta_port }}', dbname '{{ pgmeta_dbname }}', updatable 'false', passfile '/var/lib/postgresql/.pgpass')"
        - maintenance_db: {{ dbname }}
        - runas: postgres
        - require:
            - postgresql_cmd: {{ dbname }}-create-extension-postgres_fdw
        - unless:
            - -tA -c "select srvoptions @> '{\"host={{ pgmeta_host }}\",port={{ pgmeta_port }},dbname={{ pgmeta_dbname }},updatable=false,passfile=/var/lib/postgresql/.pgpass}'::text[] from pg_foreign_server where srvname = '{{ srvname }}'" | grep t

{{ dbname }}-create-{{ srvname }}-user-mapping:
    postgresql_cmd.psql_exec:
        - name: "DROP USER MAPPING IF EXISTS FOR postgres SERVER {{ srvname }}; CREATE USER MAPPING FOR postgres SERVER {{ srvname }} OPTIONS (user 'pgproxy')"
        - maintenance_db: {{ dbname }}
        - runas: postgres
        - require:
            - postgresql_cmd: {{ dbname }}-create-{{ srvname }}-foreign-server
        - unless:
            - -tA -c "select umoptions @> '{user=pgproxy}'::text[] from pg_user_mapping um join pg_foreign_server fs on um.umserver = fs.oid where srvname = '{{ srvname }}'" | grep t

{% for table_name in ['clusters', 'parts'] %}
{{ dbname }}-{{ srvname }}-import-{{table_name}}:
    postgresql_cmd.psql_exec:
        - name: "IMPORT FOREIGN SCHEMA public LIMIT TO ( {{table_name}} ) FROM SERVER {{ srvname }} INTO public"
        - maintenance_db: {{ dbname }}
        - runas: postgres
        - require:
            - postgresql_cmd: {{ dbname }}-create-{{ srvname }}-user-mapping
        - require_in:
            - postgresql_schema: {{ dbname + '-schemas-apply' }}
        - unless:
            - -tA -c "select ftoptions @> '{schema_name=public,table_name={{ table_name }}}'::text[] from pg_foreign_table ft join pg_foreign_server fs on ft.ftserver = fs.oid where fs.srvname = '{{ srvname }}' and ft.ftrelid = '{{ table_name }}'::regclass;" | grep t
{% endfor %}
{% endif %}

{% if not 'components.pg-dbs.s3db' in salt['pillar.get']('data:runlist', '') %}
s3-user:
  group.present:
    - name: s3
    - system: True
  user.present:
    - name: s3
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - s3
    - require:
        - group: s3-user

/home/s3/.pgpass:
  file.managed:
    - source: salt://{{ slspath }}/conf/pgpass
    - template: jinja
    - user: s3
    - group: s3
    - mode: 600
    - require:
      - user: s3-user

/var/log/s3:
    file.directory:
        - user: s3
        - group: s3
        - require:
            - user: s3-user

/var/run/s3:
    file.directory:
        - user: s3
        - group: s3
        - require:
            - user: s3-user

/usr/local/yandex/s3/util:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/util
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - exclude_pat: 'E@(.pyc)'
        - require:
            - user: s3-user
{% endif %}

/usr/local/yandex/s3/s3meta:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/s3meta
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - require:
            - user: s3-user

/usr/local/yandex/s3/s3_closer:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/s3_closer
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 755
        - makedirs: True
        - clean: True
        - exclude_pat: 'E@(.pyc)'
        - require:
            - user: s3-user

/usr/local/yandex/s3/sentry_sdk:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/sentry_sdk
        - user: s3
        - group: s3
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - require:
            - user: s3-user

/usr/local/yandex/s3/urllib4:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/urllib4
        - user: s3
        - group: s3
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - require:
            - user: s3-user

{% for script in ['chunk_creator', 'update_shard_stat', 'meta_update_chunks_counters', 'meta_update_buckets_usage', 'update_bucket_stat', 'chunk_mover', 'chunk_purger', 'finish_prepared_xacts', 'pg_auto_kill_s3_scripts_meta', 'update_buckets_size', 'meta_check_counters', 'smart_mover'] %}
/etc/cron.d/{{ script }}:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/{{ script }}.cron.d
        - user: root
        - group: root
        - mode: 644

/etc/logrotate.d/{{ script }}:
    file.managed:
        - source: salt://{{ slspath }}/conf/common.logrotate
        - template: jinja
        - defaults:
            script: {{ script }}
        - mode: 644
        - user: root
        - group: root
{% endfor %}

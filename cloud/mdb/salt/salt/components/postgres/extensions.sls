{% from "components/postgres/pg.jinja" import pg with context %}

{% set mandatory_extensions = ['pg_stat_statements', 'logerrors'] %}

{% if pg.version.major_num < 1400 %}
{% if salt['pillar.get']('data:perf_diag:enable', False) %}
{% do mandatory_extensions.append('mdb_perf_diag') %}
{% endif %}
{% endif %}

{% if salt['pillar.get']('data:use_pgsync', True) %}
{% do mandatory_extensions.append('lwaldump') %}
{% endif %}

{% set updatable_extensions = {'logerrors': '2.0'} %}

{% set pg_repack_version = '1.4.6' %}
{% if pg.version.major_num == 1400 %}
{% set pg_repack_version = '1.4.7' %}
{% endif %}

{% set dbs_extensions = {
    'pg_repack': {'version': pg_repack_version, 'recreate': True}
}%}

{% for extname, extdata in dbs_extensions.items() %}
pg-extension-{{ extname }}-postgres:
    mdb_postgresql.extension_present_dbs:
        - name: {{ extname }}
        - version: {{ extdata['version'] }}
        - recreate: {{ extdata.get('recreate', False) }}
{% endfor %}

{# logerrors_sql_path - path of sql file, that contains in version of pkg that can create/update extension to updatable_extensions['logerrors'] or higher #}

{% if updatable_extensions['logerrors'] == '1.0' %}
{% set logerrors_sql_path = '/usr/share/postgresql/{0}/extension/logerrors--1.0.sql'.format("{0}".format(pg.version.major_num)[:2]) %}
{% elif updatable_extensions['logerrors'] == '1.1' %}
{% set logerrors_sql_path = '/usr/share/postgresql/{0}/extension/logerrors--1.0--1.1.sql'.format("{0}".format(pg.version.major_num)[:2]) %}
{% elif updatable_extensions['logerrors'] == '2.0' %}
{% set logerrors_sql_path = '/usr/share/postgresql/{0}/extension/logerrors--1.1--2.0.sql'.format("{0}".format(pg.version.major_num)[:2]) %}
{% endif %}

{% set extensions = salt['pillar.get']('data:config:shared_preload_libraries', '').split(',') %}
{% for i in mandatory_extensions %}
{% if i not in extensions %}
{% do extensions.insert(0, i) %}
{% endif %}
{% endfor %}
{% for i in ['repl_mon', 'auto_explain', 'pg_pathman'] %}
{% if i in extensions %}
{% do extensions.remove(i) %}
{% endif %}
{% endfor %}
{% for extension in extensions %}
{% if extension %}
pg-extension-{{ extension }}-postgres:
    mdb_postgresql.extension_present:
        - name: {{ extension }}
{% if extension in updatable_extensions %}
        - version: {{ updatable_extensions[extension] }}
{% endif %}
{# Do update/create only if logerrors_sql_path exist and time of changing logerrors_sql_path (var x below) less than time of last postgres restart. #}
{# That means that code of pkg is already used by bgworker and update/create should work fine. #}
{% if extension=='logerrors' %}
        - onlyif:
            - ls {{logerrors_sql_path}} &&
             x=$(stat {{logerrors_sql_path}} | grep Change | awk '{print $2 " " $3}') &&
             sudo -u postgres psql -c "select to_timestamp('${x::19}', 'YYYY-MM-DD HH24:MI:SS') < pg_postmaster_start_time()" | grep " t"
{% endif %}
        - require:
            - cmd: postgresql-service
        - require_in:
            - test: postgresql-ready
{% endif %}
{% endfor %}

{% if 'pg_stat_wait' in salt['pillar.get']('data:config:shared_preload_libraries', '') %}
{% if salt['grains.get']('pg:role', 'master') == 'master' %}
pg_stat_wait_profile-monitor-permission:
    postgresql_cmd.psql_exec:
        - name: grant select on pg_stat_wait_profile to monitor
        - unless:
            - -tA -c "select has_table_privilege('monitor', 'public.pg_stat_wait_profile', 'select');" | grep -q "^t"
        - require:
            - cmd: postgresql-service
            - cmd: pg-extension-pg_stat_wait-postgres
            - mdb_postgres_user: postgres-user-monitor
{% endif %}
{% endif %}

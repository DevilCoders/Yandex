{% from "components/postgres/pg.jinja" import pg with context %}

{% set dbname = 's3db' %}
{% set target = salt['pillar.get']('data:s3db:target', 'latest') %}
{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}
{% set code_versions = salt['pillar.get']('data:s3db:versions', [1, ]) %}


{% if pg.version.major_num < 1200 %}
pathman:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-pathman
        - version: 1.5-3yandex.afdf2f5
        - require_in:
            - service: postgresql-service
{% endif %}


{# Remember that the order is important! #}
{%
    set sqls = [
        'common/dynamic_query.sql',
        'common/is_master.sql',
        'common/pgcheck_poll.sql'
    ]
%}

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
        - exclude_pat: 'E@(.git)|(tests)|(docker)'
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
                - {{ path + dbname }}/v{{ version }}/util
                - {{ path + dbname }}/v{{ version }}/grants
{% endfor %}
        - require:
            - postgres_database: create_db_{{ dbname }}
            - pkg: pgmigrate-pkg
            - file: {{ path + dbname }}
{% for sql in sqls %}
            - file: {{ path + sql }}
{% if pg.version.major_num < 1200 %}
            - pkg: pathman
{% endif %}
{% endfor %}
{% endif %}

{% set dbname = 'dbm' %}
{% set target = salt['pillar.get']('data:dbm:target', 'latest') %}
{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}

include:
    - components.monrun2.dbmdb
    - .mdb_metrics
    - .cron

{{ path + dbname }}:
    file.recurse:
        - source: salt://components/pg-code/dbm
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
        - conn: 'host=localhost\ dbname={{ dbname }}\ user=postgres\ connect_timeout=1'
        - termination_interval: 0.1
{% if salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - noop: True
{% endif %}
        - callbacks:
            afterAll:
                - {{ path + dbname }}/code
                - {{ path + dbname }}/grants
        - require:
            - postgres_database: create_db_{{ dbname }}
            - pkg: pgmigrate-pkg
            - file: {{ path + dbname }}
{% endif %}

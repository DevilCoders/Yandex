{% set dbname = 'katandb' %}
{% set target = salt['pillar.get']('data:katandb:target', 'latest') %}
{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}

{{ path + dbname }}:
    file.recurse:
        - source: salt://components/pg-code/katandb
        - user: postgres
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - exclude_pat: E@(tests)|(recipe)|(.svn)
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

change_db_owner_package_{{ dbname }}:
    pkg.installed:
        - name: pg-change-owner
        - version: 1.8988118
        - require:
            - postgres_database: create_db_{{ dbname }}

change_db_owner_{{ dbname }}:
    cmd.run:
        - name: /opt/yandex/pg_change_owner/bin/pg_change_owner --user katandb_admin --database katandb -s katan -c $DSN
        - env:
            - DSN: "postgresql://localhost/katandb?sslrootcert=/etc/postgresql/ssl/allCAs.pem"
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgres_database: create_db_{{ dbname }}
        - require:
            - pkg: change_db_owner_package_{{ dbname }}
            - postgres_database: create_db_{{ dbname }}

{{ dbname + '-schemas-apply' }}:
    postgresql_schema.applied:
        - name: {{ dbname }}
        - target: {{ target }}
        - conn: 'host=localhost\ dbname={{ dbname }}\ user=postgres\ connect_timeout=1'
        - termination_interval: 0.1
{% if salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - noop: True
{% endif %}
        - require:
            - postgres_database: create_db_{{ dbname }}
            - pkg: pgmigrate-pkg
            - file: {{ path + dbname }}
{% endif %}

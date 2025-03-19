{% from "components/postgres/pg.jinja" import pg with context %}
{% set dbname = 'dbaas_metadb' %}
{% set target = salt['pillar.get']('data:dbaas_metadb:target', 'latest') %}
{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}
{% set code_path = path + dbname %}
{% set vr_env = salt['pillar.get']('data:dbaas_metadb:valid_resources_env') %}
{% if salt['pillar.get']('data:dbaas_metadb:enable_cleaner', True) %}
include:
    - .dbaas_cleaner
{% endif %}

{{ code_path }}:
    file.directory:
        - user: postgres
        - makedirs: True
        - mode: 755
        - require:
            - cmd: postgresql-service

{{ code_path }}/migrations:
    file.recurse:
        - source: salt://components/pg-code/{{ dbname }}/migrations
        - user: postgres
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - require:
            - file: {{ code_path }}
        - require_in:
            - file: {{ code_path }}/migrations.yml

populate-table:
    file.managed:
        - name: {{ code_path }}/bin/populate_table.py
        - source: salt://components/pg-code/{{ dbname }}/bin/populate_table.py
        - mode: 744
        - makedirs: True
        - user: postgres
        - require:
              - file: {{ code_path }}
        - require_in:
              - file: {{ code_path }}/migrations.yml

postgresql-plpython3:
    pkg.installed:
        - name: postgresql-plpython3-{{ pg.version.major }}
        - version: {{ pg.version.pkg }}
        - require:
            - cmd: postgresql-service

{{ code_path }}/migrations.yml:
    file.managed:
        - source: salt://components/pg-code/{{ dbname }}/migrations.yml
        - user: postgres
        - mode: 744
        - require:
            - file: {{ code_path }}

{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
{%
    set metrics_confs = [
        'dbaas_metadb_quotas.conf',
        'dbaas_metadb_stats.conf',
        'dbaas_metadb_tasks.conf',
        'dataproc_metadb_jobs.conf',
    ]
%}
{% else %}
{% set metrics_confs = [] %}
{% endif %}

{% for m_conf in metrics_confs %}
/etc/mdb-metrics/conf.d/available/{{ m_conf }}:
    file.managed:
        - source: salt://{{ slspath }}/conf/{{ m_conf }}
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/{{ m_conf }}
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/{{ m_conf }}:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/{{ m_conf }}
        - watch_in:
            - service: mdb-metrics-service
{% endfor %}

{{ code_path }}_flavor_type.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/flavor_type.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_flavors.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/flavors.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_regions.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/regions.json
        - mode: 644
        - require:
              - file: {{ code_path }}

{{ code_path }}_geos.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/geos.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_disk_type_ids.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/disk_type_ids.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_default_pillar.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/default_pillar.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_cluster_type_pillars.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/cluster_type_pillars.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_role_pillars.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/role_pillars.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_default_feature_flags.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/default_feature_flags.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_valid_resources.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/valid_resources/{{ vr_env }}.json
        - mode: 644
        - show_changes: False
        - require:
            - file: {{ code_path }}

{{ code_path }}_default_versions.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/default_versions.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_default_alert.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/default_alert.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{{ code_path }}_config_host_access_ids.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/config_host_access_ids.json
        - mode: 644
        - require:
            - file: {{ code_path }}

{% if salt['grains.get']('pg') and 'role' in salt['grains.get']('pg').keys() and salt['grains.get']('pg')['role'] == 'master' %}

create_db_{{ dbname }}:
    postgres_database.present:
        - name: {{ dbname }}
        - user: postgres
        - require:
            - cmd: postgresql-service
            - mdb_postgresql: pg_sync_users
            - mdb_postgresql: pg_sync_users

change_db_owner_package_{{ dbname }}:
    pkg.installed:
        - name: pg-change-owner
        - version: 1.8988118
        - require:
            - postgres_database: create_db_{{ dbname }}

change_db_owner_{{ dbname }}:
    cmd.run:
        - name: /opt/yandex/pg_change_owner/bin/pg_change_owner --user metadb_admin --database dbaas_metadb -s dbaas -s code -c $DSN --extra "CREATE EXTENSION IF NOT EXISTS plpython3u" --extra "UPDATE pg_language SET lanpltrusted = true WHERE lanname = 'plpython3u'" --extra "CREATE EXTENSION IF NOT EXISTS pgcrypto"
        - env:
            - DSN: "postgresql://localhost/dbaas_metadb?sslrootcert=/etc/postgresql/ssl/allCAs.pem"
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgres_database: create_db_{{ dbname }}
        - require:
            - pkg: change_db_owner_package_{{ dbname }}
            - postgres_database: create_db_{{ dbname }}
            - {{ dbname }}-schemas-apply

create_repl_mon_fdw_{{ dbname }}:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/repl_mon.sql
        - maintenance_db: {{ dbname }}
        - require:
            - postgres_database: create_db_{{ dbname }}
            - file: /usr/local/yandex/sqls/repl_mon.sql
        - unless:
            - -c "SELECT c.relname FROM pg_catalog.pg_foreign_table ft INNER JOIN pg_catalog.pg_class c ON c.oid = ft.ftrelid;" | grep repl_mon

{{ dbname }}-schemas-apply:
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
            - file: {{ code_path }}/migrations.yml
            - pkg: postgresql-plpython3

{% set dsn = 'host=localhost port=5432 dbname=' + dbname %}

{{ dbname }}-flavor-type-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_flavor_type.json' }} -t dbaas.flavor_type -k id
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_flavor_type.json' }}
            - file: populate-table

{{ dbname }}-flavors-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_flavors.json' }} -t dbaas.flavors
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_flavors.json' }}
            - file: populate-table

{{ dbname }}-regions-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_regions.json' }} -t dbaas.regions
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
              - postgresql_schema: {{ dbname +'-schemas-apply' }}
              - file: {{ path + dbname + '_regions.json' }}
              - file: populate-table

{{ dbname }}-geos-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_geos.json' }} -t dbaas.geo
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_geos.json' }}
            - file: populate-table
        - require:
              - cmd: {{ dbname +'-regions-set' }}

{{ dbname }}-disk-type-ids-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_disk_type_ids.json' }} -t dbaas.disk_type -k disk_type_ext_id
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_disk_type_ids.json' }}
            - file: populate-table

{{ dbname }}-default-pillar-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_default_pillar.json' }} -t dbaas.default_pillar -k id
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_default_pillar.json' }}
            - file: populate-table

{{ dbname }}-cluster-type-pillars-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_cluster_type_pillars.json' }} -t dbaas.cluster_type_pillar -k type
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_cluster_type_pillars.json' }}
            - file: populate-table

{{ dbname }}-role-pillars-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_role_pillars.json' }} -t dbaas.role_pillar --refill
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_role_pillars.json' }}
            - file: populate-table

{{ dbname }}-default-feature-flags-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_default_feature_flags.json' }} -t dbaas.default_feature_flags -k flag_name
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_default_feature_flags.json' }}
            - file: populate-table

{{ dbname }}-valid-resources-set:
    cmd.run:
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_valid_resources.json' }} -t dbaas.valid_resources -k id --refill
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_valid_resources.json' }}
            - file: populate-table
        - require:
            - cmd: {{ dbname +'-disk-type-ids-set' }}
            - cmd: {{ dbname +'-flavors-set' }}
            - cmd: {{ dbname +'-geos-set' }}

{{ dbname }}-default-versions-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_default_versions.json' }} -t dbaas.default_versions -k type,component,name,env
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_default_versions.json' }}
            - file: populate-table
        - require:
            - file: {{ code_path }}_default_versions.json

{{ dbname }}-default-alert-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_default_alert.json' }} -t dbaas.default_alert -k template_id
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_default_alert.json' }}
            - file: populate-table
        - require:
            - file: {{ code_path }}_default_alert.json

{{ dbname }}-config_host_access_ids-set:
    cmd.run:
{% if not salt['dbaas.pillar']('data:migrates_in_k8s', False) %}
        - name: python2 {{ path + dbname + '/bin/populate_table.py' }} -d '{{ dsn }}' -f {{ path + dbname + '_config_host_access_ids.json' }} -t dbaas.config_host_access_ids -k access_id --refill
{% else %}
        - name: "true"
{% endif %}
        - runas: postgres
        - group: postgres
        - onchanges:
            - postgresql_schema: {{ dbname +'-schemas-apply' }}
            - file: {{ path + dbname + '_config_host_access_ids.json' }}
            - file: populate-table

{% endif %}

/etc/cron.yandex/backup_retry_delete.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/backup_retry_delete.py
        - mode: 755
        - user: root

/etc/cron.d/backup_retry_delete:
    file.managed:
        - source: salt://{{ slspath }}/conf/backup_retry_delete.cron
        - mode: 644
        - user: root

/etc/logrotate.d/backup_retry_delete:
    file.managed:
        - source: salt://{{ slspath }}/conf/backup_retry_delete.logrotate
        - mode: 644
        - user: root

/var/log/backup-retry-delete.log:
    file.managed:
        - replace: false
        - mode: 644
        - user: postgres
        - group: postgres

{% from "components/postgres/pg.jinja" import pg with context %}

pg-init:
    postgresql_cmd.master_init:
        - name: {{ pg.data }}
        - version: {{ pg.version.major }}
{% if salt['pillar.get']('data:config:pgusers:postgres:password', False) %}
        - pwfile: {{ pg.prefix }}/.pwfile
{% endif %}
        - require:
            - file: {{ pg.prefix }}/.pwfile
            - pkg: postgresql{{ pg.version.major }}-server
        - require_in:
            - file: /etc/postgresql/{{ pg.version.major }}/data
            - service: postgresql-service

{{ pg.data }}/conf.d:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - pkg: postgresql{{ pg.version.major }}-server
            - postgresql_cmd: pg-init
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - file: {{ pg.data }}/conf.d/pg_hba.conf

{% if salt['pillar.get']('restore-from:cid') %}{# RESTORE-FROM #}

/tmp/recovery-state:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0750
        - makedirs: True


recovery_conf_file:
    file.managed:
{% if pg.version.major_num >= 1200 %}
        - name: {{ pg.data }}/conf.d/recovery.conf
{% else %}
        - name: {{ pg.data }}/recovery.conf
{% endif %}
        - user: postgres
        - group: postgres
        - contents: |
            recovery_target_action = 'promote'
            restore_command = '/usr/bin/timeout -s SIGQUIT 60 /usr/bin/wal-g wal-fetch "%f" "%p" --config /etc/wal-g/wal-g-restore.yaml'
{% if not salt['pillar.get']('restore-from:restore-latest', False) %}
            recovery_target_time = '{{ salt['pillar.get']('restore-from:time') }}'
            recovery_target_inclusive = {{ {True: 'true', False: 'false'}[salt['pillar.get']('restore-from:time-inclusive')] }}
{% endif %}
        - require:
             - cmd: backup-fetched

{% if pg.version.major_num >= 1200 %}
recovery_signal:
    file.managed:
        - name: {{ pg.data }}/recovery.signal
        - user: postgres
        - group: postgres
{% endif %}

/usr/local/yandex/pg_restore_from_backup.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pg_restore_from_backup.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

postgresql-restore:
    cmd.run:
        - name: >
            python /usr/local/yandex/pg_restore_from_backup.py
            --recovery-state=/tmp/recovery-state
            --pg-bin={{ pg.bin_path }}
            --pg-data={{ pg.data }}
            --pg-config={{ pg.config_file_path }}
            --pg-version={{ pg.version.major_num }}
            --rename-databases-from-to='{{ salt['pillar.get']('restore-from:rename-database-from-to') | json }}'
            --old-postgres-password={{ salt['pillar.get']('data:restore-from-pillar-data:config:pgusers:postgres:password', '') | yaml_encode }}
            --new-postgres-password={{ salt['pillar.get']('data:config:pgusers:postgres:password', '') | yaml_encode }}
        - runas: postgres
        - group: postgres
        - require:
            - file: /usr/local/yandex/pg_restore_from_backup.py
            - pkg: python-psycopg2
            - pkg: python3-psycopg2
            - file: /tmp/recovery-state
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - file: {{ pg.data }}/conf.d/pg_hba.conf
            - file: recovery_conf_file
            - file: /var/log/postgresql
            - cmd: /etc/sysctl.d/postgres.conf
            - cmd: backup-fetched
            - cmd: locale-gen
{% if pg.version.major_num >= 1200 %}
            - file: recovery_signal
{% endif %}
        - require_in:
            - service: postgresql-service
{% else %}
{% if pg.version.major_num >= 1200 %}
{# MDB-6684: prevent skipping missing configuration file error #}
recovery_conf_file:
    file.managed:
        - name: {{ pg.data }}/conf.d/recovery.conf
        - user: postgres
        - group: postgres
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
{% endif %}
{% endif %}

include:
    - .service

extend:
    postgresql-service:
        service.running:
            - require:
                - file: {{ pg.data }}/conf.d/postgresql.conf
                - file: {{ pg.data }}/conf.d/pg_hba.conf
                - file: /var/log/postgresql
                - file: {{ pg.prefix }}/.pgpass
                - file: {{ pg.prefix }}/.server-pgpass
                - cmd: /etc/sysctl.d/postgres.conf
                - cmd: locale-gen
{% if salt['pillar.get']('data:pg_ssl', True) %}
                - file: /etc/postgresql/ssl
                - file: /etc/postgresql/ssl/server.crt
                - file: /etc/postgresql/ssl/server.key
                - file: /etc/postgresql/ssl/allCAs.pem
{% endif %}
        cmd.wait:
            - require:
                - file: /usr/local/yandex/pg_wait_started.py
                - pkg: python-psycopg2
                - pkg: python3-psycopg2

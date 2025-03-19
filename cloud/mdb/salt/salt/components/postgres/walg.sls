{% from "components/postgres/pg.jinja" import pg with context %}
{% set osrelease = salt['grains.get']('osrelease') %}

include:
    - .walg-config
{% if not salt['pillar.get']('data:running_on_template_machine', False) and not salt['pillar.get']('data:s3:gpg_key') %}
    - .gpg
{% endif %}
{% if salt['pillar.get']('do-backup') %}
    - .run-backup
{% endif %}
{% if salt['pillar.get']('data:walg_periodic_backups', True) %}
    - .walg-cron
{% else %}

/etc/cron.d/pg_walg_backup_push:
    file.absent
{% endif %}

extend:
{% if salt['pillar.get']('do-backup') %}
    do_s3_backup:
        cmd.run:
            - require:
                - test: postgresql-ready
                - file: /usr/local/yandex/pg_walg_backup_push.py
                - file: /etc/wal-g-backup-push.conf
{% endif %}

    postgresql-walg-config-req:
        test.nop:
            - require:
                - pkg: walg-packages
                - file: /etc/wal-g
    postgresql-walg-config-ready:
        test.nop:
            - require_in:
                - service: postgresql-service

/etc/wal-g/envdir:
    file.absent

{% set environment = salt['pillar.get']('yandex:environment', 'dev') %}
{% if salt['pillar.get']('data:s3:gpg_key')  %}
/etc/wal-g/PGP_KEY:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:s3:gpg_key') | indent(12)}}
        - require:
            - pkg: walg-packages
            - file: /etc/wal-g/wal-g.yaml
        - require_in:
            - service: postgresql-service
{% endif %}

walg-packages:
    pkg.installed:
        - pkgs:
{% set walg_version = salt['pillar.get']('data:walg:version') %}
{% if not walg_version %}
{%     if environment == 'dev' %}
{%     set walg_version = '1266-1eb88a57' %}
{%     elif environment == 'qa' %}
{%     set walg_version = '1266-1eb88a57' %}
{%     elif environment == 'prod' %}
{%     set walg_version = '1208-c2fc0a69' %}
{%     else %}
{%     set walg_version = '1208-c2fc0a69' %}
{%     endif %}
{% endif %}
            - wal-g: {{ walg_version }}
            - daemontools
            - python3-dateutil
        - prereq_in:
            - cmd: repositories-ready
        - require_in:
            - service: postgresql-service
            - file: /etc/wal-g/wal-g.yaml

/etc/wal-g:
    file.directory:
        - user: root
        - group: s3users
        - mode: 0750
        - makedirs: True
        - require:
            - group: s3users
        - require_in:
            - file: /etc/wal-g/wal-g.yaml

/usr/local/yandex/pg_walg_backup_push.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pg_walg_backup_push.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/pg_walg_backup_delete.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pg_walg_backup_delete.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/pg_walg_delete_garbage.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pg_walg_delete_garbage.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

s3users:
    group.present:
        - members:
            - postgres
            - monitor
        - system: True
        - require:
            - file: {{ pg.data }}
            - pkg: yamail-monrun
        - watch_in:
            - service: snaked
            - service: juggler-client
        - require_in:
            - service: postgresql-service
            - file: /etc/wal-g/wal-g.yaml

{% if salt['pillar.get']('restore-from:cid') %}
/etc/wal-g/wal-g-restore.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g-restore.yaml
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - group: s3users
            - file: /etc/wal-g
            - pkg: walg-packages
        - require_in:
            - cmd: backup-fetched

/etc/wal-g/PGP_KEY_RESTORE:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:restore-from-pillar-data:s3:gpg_key') | indent(12)}}
        - require:
            - group: s3users
            - file: /etc/wal-g
            - pkg: walg-packages
            - file: /etc/wal-g/wal-g-restore.yaml
        - require_in:
            - cmd: backup-fetched

{% if not salt.pillar.get('data:backup:use_backup_service') and not salt.pillar.get('restore-from:use_backup_service_at_latest_rev') %}
backup-fetched:
    cmd.run:
        - name: >
            rm -rf {{ pg.data }} &&
            wal-g backup-fetch {{ pg.data }} {{ salt['pillar.get']('restore-from:backup-id') }} --config /etc/wal-g/wal-g-restore.yaml --turbo &&
            touch /tmp/recovery-state/backup-fetched
        - runas: postgres
        - group: postgres
        - require:
            - file: /tmp/recovery-state
        - require_in:
            - file: {{ pg.data }}/wals
            - file: {{ pg.data }}/conf.d
        - unless:
            - ls /tmp/recovery-state/backup-fetched
{% else %}
{% set bid = salt['pillar.get']('restore-from:backup-id') %}
# after MDB-13806 use id, not name --target-user-data='{"backup_id": "{{ bid }}" }'
backup-fetched:
    cmd.run:
        - name: >
            rm -rf {{ pg.data }} &&
            wal-g backup-fetch {{ pg.data }} {{ bid }} --config /etc/wal-g/wal-g-restore.yaml --turbo &&
            touch /tmp/recovery-state/backup-fetched
        - runas: postgres
        - group: postgres
        - require:
            - file: /tmp/recovery-state
        - require_in:
            - file: {{ pg.data }}/wals
            - file: {{ pg.data }}/conf.d
        - unless:
            - ls /tmp/recovery-state/backup-fetched
{% endif %}

remove-keys-when-postgres-restored:
    cmd.run:
        - name: >
            rm -f /etc/wal-g/wal-g-restore.yaml &&
            rm -f /etc/wal-g/PGP_KEY_RESTORE
        - require:
            - cmd: postgresql-restore
{% endif %}

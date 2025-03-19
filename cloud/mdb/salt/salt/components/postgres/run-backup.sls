{% if salt['pillar.get']('data:use_walg', True) %}
{% if not salt.pillar.get('data:backup:use_backup_service') %}
do_s3_backup:
    cmd.run:
        - name: >
            grep 'delta-from-previous-backup' /usr/local/yandex/pg_walg_backup_push.py &&
            flock -o /tmp/pg_walg_backup_push.lock
            /usr/local/yandex/pg_walg_backup_push.py
            --skip-delete-old
            --skip-election-in-zk
{% if salt['pillar.get']('delta_from_previous_backup', True) %}
            --delta-from-previous-backup
{% endif %}
{% if salt['pillar.get']('user_backup') %}
            --user-backup
{% endif %}
{% if salt['pillar.get']('backup_id') %}
            --id {{ salt['pillar.get']('backup_id') }}
{% endif %}
{% if salt['pillar.get']('from_backup_id') %}
            --delta-from-id {{ salt['pillar.get']('from_backup_id') }}
{% endif %}
{% if salt['pillar.get']('full_backup') %}
            --full
{% endif %}
            ||
            flock -o /tmp/pg_walg_backup_push.lock
            /usr/local/yandex/pg_walg_backup_push.py
            --skip-delete-old
            --skip-election-in-zk
        - runas: postgres
        - group: postgres
{% else %}
do_s3_backup:
    cmd.run:
        - name: >

            flock -o -n /tmp/pg_walg_backup_push.lock
            /usr/bin/wal-g backup-list --config=/etc/wal-g/wal-g.yaml --json --detail | grep {{ salt['pillar.get']('backup_id') }}
            || /usr/local/yandex/pg_walg_backup_push.py
            --skip-delete-old
            --skip-election-in-zk
{% if salt['pillar.get']('user_backup') %}
            --user-backup
{% endif %}
{% if salt['pillar.get']('backup_id') %}
            --id {{ salt['pillar.get']('backup_id') }}
{% endif %}
{% if salt['pillar.get']('from_backup_name') %}
            --delta-from-name {{ salt['pillar.get']('from_backup_name') }}
{% else %}
{% if salt['pillar.get']('from_backup_id') %}
            --delta-from-id {{ salt['pillar.get']('from_backup_id') }}
{% endif %}
{% endif %}
{% if salt['pillar.get']('full_backup') %}
            --full
{% endif %}
        - runas: postgres
        - group: postgres
{% endif %}
{% endif %}

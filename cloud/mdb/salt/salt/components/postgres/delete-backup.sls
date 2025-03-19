{% if salt['pillar.get']('data:use_walg', True) %}
delete_s3_backup:
    cmd.run:
        - name: >
            flock -o /tmp/pg_walg_backup_push.lock
            /usr/local/yandex/pg_walg_backup_delete.py
            --id {{ salt['pillar.get']('backup_id') }}
        - runas: postgres
{% endif %}

{# Configure the custom s3 prefix, if any. It is required to delete backups of the old Postgres major versions. #}
{% set s3_prefix_setting = '' %}
{% set walg_s3_prefix_setting = '' %}
{% if salt['pillar.get']('s3_prefix') %}
{% set s3_prefix_setting = '--s3-prefix ' + salt['pillar.get']('s3_prefix') %}
{% set walg_s3_prefix_setting = '--walg-s3-prefix ' + salt['pillar.get']('s3_prefix') %}
{% endif %}

do-walg-backup-delete:
    cmd.run:
{% if salt['pillar.get']('backup_name') %}
        - name: >
            /usr/bin/wal-g backup-list --config=/etc/wal-g/wal-g.yaml {{ walg_s3_prefix_setting }} | grep {{ salt['pillar.get']('backup_name') }} &&
            /usr/local/yandex/pg_walg_backup_delete.py {{ s3_prefix_setting }} --name {{ salt['pillar.get']('backup_name') }} || true
{% else %}
        - name: >
            /usr/bin/wal-g backup-list --detail --json --config=/etc/wal-g/wal-g.yaml {{ walg_s3_prefix_setting }} | grep {{ salt['pillar.get']('backup_id') }} &&
            /usr/local/yandex/pg_walg_backup_delete.py {{ s3_prefix_setting }} --id {{ salt['pillar.get']('backup_id') }} || true
{% endif %}
        - runas: postgres
        - group: postgres

do-walg-delete-garbage:
    cmd.run:
        - name: /usr/local/yandex/pg_walg_delete_garbage.py {{ s3_prefix_setting }}
        - runas: postgres
        - group: postgres


/etc/yandex/ch-backup/ch-backup-restore.conf:
    fs.file_present:
        - contents_function: mdb_clickhouse.backup_config_for_restore
        - contents_format: yaml
        - mode: 644
        - makedirs: True
        - require:
            - test: ch-backup-config-ready

restore-ch-from-backup:
    cmd.run:
        - name: >
            {{ salt.mdb_clickhouse.restore_command() }} && touch /tmp/restore-ch-from-backup-success
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - require:
            - fs: /etc/yandex/ch-backup/ch-backup-restore.conf
            - cmd: clickhouse-server
        - require_in:
            - test: clickhouse-sync-databases-req
{% for database in salt.pillar.get('data:clickhouse:schemas', []) %}
            - cmd: apply_{{ database }}
{% endfor %}
        - unless:
            - ls /tmp/restore-ch-from-backup-success

{% if salt.pillar.get('data:clickhouse:sql_user_management', False) %}
restore-ch-access-control:
    cmd.run:
            - name: >
                ch-backup -c /etc/yandex/ch-backup/ch-backup-restore.conf
                restore-access-control
                {{ salt.pillar.get('restore-from:backup-id') }} &&
                touch /tmp/restore-ch-access_control-from-backup-success
            - env:
                - LC_ALL: C.UTF-8
                - LANG: C.UTF-8
            - require:
                - fs: /etc/yandex/ch-backup/ch-backup-restore.conf
                - cmd: clickhouse-server
            - require_in:
                - cmd: remove-ch-backup-restore-conf
            - unless:
                - ls /tmp/restore-ch-access_control-from-backup-success

clickhouse-restore-server-restart:
    cmd.run:
        - name: >
            service clickhouse-server restart &&
            timeout 10 bash -c "while ps -efHww | grep -qE 'clickhouse-server\s+--daemon'; do sleep 1; done" &&
            /usr/local/yandex/ch_wait_started.py -q
        - require:
            - cmd: restore-ch-access-control
        - require_in:
            - test: clickhouse-sql-users-req
            - test: clickhouse-sync-databases-req
            - cmd: remove-ch-backup-restore-conf
{% endif %}

remove-ch-backup-restore-conf:
    cmd.run:
        - name: rm -f /etc/yandex/ch-backup/ch-backup-restore.conf
        - require:
            - cmd: restore-ch-from-backup
